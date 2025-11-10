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
#include "lite_gdi32.h"
#include "lite_uxtheme.h"
#include "lite_normaliz.h"

#include "categories.h"
#include "list_operations.h"
#include "string_tables.h"
#include "folder_browser.h"

#include "site_manager_utilities.h"
#include "cmessagebox.h"

#include "dark_mode.h"

#define SM_COLUMN_NUM						0
#define SM_COLUMN_SITE						1
#define SM_COLUMN_CATEGORY					2
#define SM_COLUMN_DOWNLOAD_DIRECTORY		3
#define SM_COLUMN_DOWNLOAD_PARTS			4
#define SM_COLUMN_DOWNLOAD_SPEED_LIMIT		5
#define SM_COLUMN_SSL_TLS_VERSION			6
#define SM_COLUMN_USERNAME					7
#define SM_COLUMN_PASSWORD					8
#define SM_COLUMN_DOWNLOAD_OPERATIONS		9
#define SM_COLUMN_COMMENTS					10
#define SM_COLUMN_COOKIES					11
#define SM_COLUMN_HEADERS					12
#define SM_COLUMN_POST_DATA					13
#define SM_COLUMN_PROXY_TYPE				14
#define SM_COLUMN_PROXY_SERVER				15
#define SM_COLUMN_PROXY_PORT				16
#define SM_COLUMN_PROXY_USERNAME			17
#define SM_COLUMN_PROXY_PASSWORD			18
#define SM_COLUMN_RESOLVE_DOMAIN_NAMES		19





#define BTN_NEW_SITE						1000
#define BTN_SAVE_SITE						1001
#define BTN_REMOVE_SITE						1002

#define CHK_SHOW_PASSWORDS					1004
#define BTN_CLOSE_SITE_MANAGER_WND			1005

#define EDIT_SITE							1006
#define EDIT_USERNAME_SITE					1007
#define EDIT_PASSWORD_SITE					1008

#define CHK_SM_SEND_DATA					1009

#define CB_SM_CATEGORY						1010

#define CHK_SM_ENABLE_DOWNLOAD_DIRECTORY	1011
#define BTN_SM_DOWNLOAD_DIRECTORY			1012

#define CHK_SM_ENABLE_DOWNLOAD_PARTS		1013
#define EDIT_SM_DOWNLOAD_PARTS				1014

#define CHK_SM_ENABLE_SPEED_LIMIT			1015
#define EDIT_SM_SPEED_LIMIT					1016

#define CHK_SM_SIMULATE_DOWNLOAD			1017
#define CB_SM_DOWNLOAD_OPERATION			1018

//

#define CB_SM_PROXY_TYPE				1019

#define BTN_SM_TYPE_HOST_SOCKS			1020
#define BTN_SM_TYPE_IP_ADDRESS_SOCKS	1021
#define EDIT_SM_HOST_SOCKS				1022
#define EDIT_SM_IP_ADDRESS_SOCKS		1023
#define EDIT_SM_PORT_SOCKS				1024

#define EDIT_SM_PROXY_AUTH_USERNAME			1025
#define EDIT_SM_PROXY_AUTH_PASSWORD			1026

#define EDIT_SM_AUTH_IDENT_USERNAME_SOCKS	1027

#define BTN_SM_RESOLVE_DOMAIN_NAMES_V4A	1028

#define BTN_SM_AUTHENTICATION_SOCKS	1029

#define EDIT_SM_AUTH_USERNAME_SOCKS	1030
#define EDIT_SM_AUTH_PASSWORD_SOCKS	1031

#define BTN_SM_RESOLVE_DOMAIN_NAMES	1032

//

#define MENU_SM_ENABLE_SEL			2000
#define MENU_SM_DISABLE_SEL			2001
#define MENU_SM_REMOVE_SEL			2002
#define MENU_SM_SELECT_ALL			2003

HWND g_hWnd_site_manager = NULL;
HWND g_hWnd_site_list = NULL;


HWND g_hWnd_sm_tab = NULL;


HWND g_hWnd_static_sm_category = NULL;
HWND g_hWnd_sm_category = NULL;

HWND g_hWnd_chk_sm_enable_download_directory = NULL;
HWND g_hWnd_sm_download_directory = NULL;
HWND g_hWnd_btn_sm_download_directory = NULL;

HWND g_hWnd_static_sm_comments = NULL;
HWND g_hWnd_edit_sm_comments = NULL;

HWND g_hWnd_static_sm_cookies = NULL;
HWND g_hWnd_edit_sm_cookies = NULL;

HWND g_hWnd_static_sm_headers = NULL;
HWND g_hWnd_edit_sm_headers = NULL;

HWND g_hWnd_chk_sm_send_data = NULL;
HWND g_hWnd_edit_sm_data = NULL;


HWND g_hWnd_chk_sm_enable_download_parts = NULL;
HWND g_hWnd_sm_download_parts = NULL;
HWND g_hWnd_ud_sm_download_parts = NULL;

HWND g_hWnd_static_sm_ssl_version = NULL;
HWND g_hWnd_sm_ssl_version = NULL;


HWND g_hWnd_chk_sm_enable_speed_limit = NULL;
HWND g_hWnd_sm_speed_limit = NULL;


HWND g_hWnd_static_sm_site = NULL;
HWND g_hWnd_edit_sm_site = NULL;

HWND g_hWnd_btn_sm_authentication = NULL;
HWND g_hWnd_static_sm_username = NULL;
HWND g_hWnd_edit_sm_username = NULL;
HWND g_hWnd_static_sm_password = NULL;
HWND g_hWnd_edit_sm_password = NULL;

HWND g_hWnd_chk_sm_simulate_download = NULL;
HWND g_hWnd_static_sm_download_operation = NULL;
HWND g_hWnd_sm_download_operation = NULL;

HWND g_hWnd_new_site = NULL;
HWND g_hWnd_save_site = NULL;
HWND g_hWnd_remove_site = NULL;

HWND g_hWnd_chk_show_passwords = NULL;
HWND g_hWnd_close_lm_wnd = NULL;

HMENU g_hMenuSub_site_manager = NULL;

//////

HWND g_hWnd_static_sm_proxy_type = NULL;
HWND g_hWnd_sm_proxy_type = NULL;

HWND g_hWnd_static_sm_hoz1 = NULL;

HWND g_hWnd_sm_ip_address_socks = NULL;
HWND g_hWnd_sm_hostname_socks = NULL;
HWND g_hWnd_sm_port_socks = NULL;

HWND g_hWnd_static_sm_port_socks = NULL;
HWND g_hWnd_static_sm_colon_socks = NULL;

HWND g_hWnd_chk_sm_type_hostname_socks = NULL;
HWND g_hWnd_chk_sm_type_ip_address_socks = NULL;

HWND g_hWnd_static_sm_proxy_auth_username = NULL;
HWND g_hWnd_edit_sm_proxy_auth_username = NULL;
HWND g_hWnd_static_sm_proxy_auth_password = NULL;
HWND g_hWnd_edit_sm_proxy_auth_password = NULL;

HWND g_hWnd_static_sm_auth_ident_username_socks = NULL;
HWND g_hWnd_sm_auth_ident_username_socks = NULL;

HWND g_hWnd_chk_sm_resolve_domain_names_v4a = NULL;

HWND g_hWnd_chk_sm_use_authentication_socks = NULL;

HWND g_hWnd_static_sm_auth_username_socks = NULL;
HWND g_hWnd_sm_auth_username_socks = NULL;
HWND g_hWnd_static_sm_auth_password_socks = NULL;
HWND g_hWnd_sm_auth_password_socks = NULL;

HWND g_hWnd_chk_sm_resolve_domain_names = NULL;

//////

unsigned char sm_t_down_speed = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.

wchar_t *t_sm_download_directory = NULL;

bool skip_site_list_draw = false;

wchar_t g_default_pw_char = 0x25CF;	// Bullet
bool g_show_passwords = false;

WNDPROC SMListProc = NULL;

WNDPROC SMProc = NULL;
WNDPROC SMTabProc = NULL;

HBRUSH g_sm_tab_brush = NULL;
int g_sm_tab_width = 0;
bool g_sm_use_theme = true;

wchar_t sm_limit_tooltip_text[ 32 ];
HWND g_hWnd_sm_limit_tooltip = NULL;

HFONT hFont_copy_sm_proxy = NULL;

SITE_INFO *g_selected_site_info = NULL;
int g_selected_site_index = -1;

bool g_sm_draw_tab_pane = false;

int site_manager_spinner_width = 0;
int site_manager_spinner_height = 0;

UINT current_dpi_site_manager = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_site_manager = 0;
HFONT hFont_site_manager = NULL;

#define _SCALE_SM_( x )						_SCALE_( ( x ), dpi_site_manager )

wchar_t *GetSiteInfoString( SITE_INFO *si, int column, int item_index, wchar_t *tbuf, unsigned short tbuf_size )
{
	wchar_t *buf = NULL;

	switch ( column )
	{
		case SM_COLUMN_NUM:
		{
			buf = tbuf;	// Reset the buffer pointer.

			__snwprintf( buf, tbuf_size, L"%lu", item_index );
		}
		break;

		case SM_COLUMN_SITE: { buf = si->w_host; } break;

		case SM_COLUMN_CATEGORY: { buf = si->category; } break;

		case SM_COLUMN_DOWNLOAD_DIRECTORY:
		{
			if ( si->download_operations & DOWNLOAD_OPERATION_SIMULATE )
			{
				buf = ST_V__Simulated_;
			}
			else if ( si->use_download_directory )
			{
				buf = si->download_directory;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_DOWNLOAD_PARTS:
		{
			if ( si->use_parts && si->parts > 0 )
			{
				buf = tbuf;	// Reset the buffer pointer.

				__snwprintf( buf, tbuf_size, L"%lu", si->parts );
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_DOWNLOAD_SPEED_LIMIT:
		{
			if ( si->use_download_speed_limit && si->download_speed_limit > 0 )
			{
				buf = tbuf;	// Reset the buffer pointer.

				unsigned int length = FormatSizes( buf, tbuf_size, sm_t_down_speed, si->download_speed_limit );
				buf[ length++ ] = L'/';
				buf[ length++ ] = L's';
				buf[ length ] = 0;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_SSL_TLS_VERSION:
		{
			switch ( si->ssl_version )
			{
				// We've offset the case because 0 will be default.
				case 1: { buf = ST_V_SSL_2_0; } break;
				case 2: { buf = ST_V_SSL_3_0; } break;
				case 3: { buf = ST_V_TLS_1_0; } break;
				case 4: { buf = ST_V_TLS_1_1; } break;
				case 5: { buf = ST_V_TLS_1_2; } break;
				case 6: { buf = ST_V_TLS_1_3; } break;
				default: { buf = L""; } break;
			}
		}
		break;

		case SM_COLUMN_USERNAME: { buf = si->w_username; } break;
		case SM_COLUMN_PASSWORD: { buf = ( g_show_passwords ? si->w_password : ST_V__PASSWORD_ ); } break;

		case SM_COLUMN_DOWNLOAD_OPERATIONS:
		{
			int buf_offset = 0;

			buf = tbuf;	// Reset the buffer pointer.

			if ( si->download_operations & DOWNLOAD_OPERATION_SIMULATE )
			{
				buf_offset = __snwprintf( buf, tbuf_size, L"%s", ST_V_Simulate );
			}

			if ( si->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
			{
				if ( buf_offset > 0 )
				{
					buf[ buf_offset++ ] = L',';
					buf[ buf_offset++ ] = L' ';
				}

				buf_offset = __snwprintf( buf + buf_offset, tbuf_size - buf_offset, L"%s", ST_V_Add );
			}
			else if ( si->download_operations & DOWNLOAD_OPERATION_VERIFY )
			{
				if ( buf_offset > 0 )
				{
					buf[ buf_offset++ ] = L',';
					buf[ buf_offset++ ] = L' ';
				}

				buf_offset = __snwprintf( buf + buf_offset, tbuf_size - buf_offset, L"%s", ST_V_Verify );
			}
			else
			{
				if ( buf_offset > 0 )
				{
					buf[ buf_offset++ ] = L',';
					buf[ buf_offset++ ] = L' ';
				}

				buf_offset = __snwprintf( buf + buf_offset, tbuf_size - buf_offset, L"%s", ST_V_Download );
			}

			if ( buf_offset == 0 )
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_COMMENTS:
		{
			if ( si->comments != NULL )
			{
				buf = tbuf;	// Reset the buffer pointer.

				// tbuf_size is always 128, so this will never happen.
				//if ( tbuf_size <= 4 )
				//{
				//	buf = L"...";
				//}
				//else
				//{
					int buf_length = __snwprintf( buf, tbuf_size, L"%.*s", tbuf_size, si->comments );
					if ( buf_length >= tbuf_size )
					{
						buf[ tbuf_size - 4 ] = L'.';
						buf[ tbuf_size - 3 ] = L'.';
						buf[ tbuf_size - 2 ] = L'.';
						buf[ tbuf_size - 1 ] = 0;	// Sanity.
					}
				//}
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_COOKIES:
		{
			if ( si->utf8_cookies != NULL )
			{
				buf = ST_V__DATA_;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_HEADERS:
		{
			if ( si->utf8_headers != NULL )
			{
				buf = ST_V__DATA_;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_POST_DATA:
		{
			if ( si->method == METHOD_POST )
			{
				if ( si->utf8_data != NULL )
				{
					buf = ST_V__DATA_;
				}
				else
				{
					buf = ST_V_Empty_Body;
				}
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_PROXY_TYPE:
		{
			switch ( si->proxy_info.type )
			{
				case 1: { buf = ST_V_HTTP; } break;
				case 2: { buf = ST_V_HTTPS; } break;
				case 3: { buf = ST_V_SOCKS_v4; } break;
				case 4: { buf = ST_V_SOCKS_v5; } break;
				default: { buf = L""; } break;
			}
		}
		break;

		case SM_COLUMN_PROXY_SERVER:
		{
			if ( si->proxy_info.type != 0 )
			{
				if ( si->proxy_info.address_type == 0 )
				{
					if ( si->proxy_info.hostname != NULL )
					{
						buf = si->proxy_info.hostname;
					}
					else
					{
						buf = L"";
					}
				}
				else
				{
					buf = tbuf;	// Reset the buffer pointer.

					__snwprintf( buf, tbuf_size, L"%hu.%hu.%hu.%hu", FIRST_IPADDRESS( si->proxy_info.ip_address ),
																	 SECOND_IPADDRESS( si->proxy_info.ip_address ),
																	 THIRD_IPADDRESS( si->proxy_info.ip_address ),
																	 FOURTH_IPADDRESS( si->proxy_info.ip_address ) );
				}
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_PROXY_PORT:
		{
			if ( si->proxy_info.type != 0 )
			{
				buf = tbuf;	// Reset the buffer pointer.

				__snwprintf( buf, tbuf_size, L"%hu", si->proxy_info.port );
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_PROXY_USERNAME:
		{
			if ( si->proxy_info.w_username != NULL )
			{
				buf = si->proxy_info.w_username;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_PROXY_PASSWORD:
		{
			if ( si->proxy_info.w_password != NULL )
			{
				buf = ( g_show_passwords ? si->proxy_info.w_password : ST_V__PASSWORD_ );
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case SM_COLUMN_RESOLVE_DOMAIN_NAMES:
		{
			if ( si->proxy_info.type == 3 || si->proxy_info.type == 4 )
			{
				if ( si->proxy_info.resolve_domain_names )
				{
					buf = ST_V_Yes;
				}
				else
				{
					buf = ST_V_No;
				}
			}
			else
			{
				buf = L"";
			}
		}
		break;
	}

	return buf;
}

void ShowHideSMProxyWindows( int index )
{
	if ( index == 0 )
	{
		_ShowWindow( g_hWnd_static_sm_hoz1, SW_HIDE );

		_ShowWindow( g_hWnd_sm_port_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_sm_port_socks, SW_HIDE );
		_ShowWindow( g_hWnd_static_sm_colon_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_sm_type_hostname_socks, SW_HIDE );
		_ShowWindow( g_hWnd_chk_sm_type_ip_address_socks, SW_HIDE );

		_ShowWindow( g_hWnd_sm_ip_address_socks, SW_HIDE );
		_ShowWindow( g_hWnd_sm_hostname_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_sm_proxy_auth_username, SW_HIDE );
		_ShowWindow( g_hWnd_edit_sm_proxy_auth_username, SW_HIDE );
		_ShowWindow( g_hWnd_static_sm_proxy_auth_password, SW_HIDE );
		_ShowWindow( g_hWnd_edit_sm_proxy_auth_password, SW_HIDE );

		_ShowWindow( g_hWnd_static_sm_auth_ident_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_sm_auth_ident_username_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_sm_resolve_domain_names_v4a, SW_HIDE );

		_ShowWindow( g_hWnd_chk_sm_use_authentication_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_sm_auth_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_sm_auth_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_static_sm_auth_password_socks, SW_HIDE );
		_ShowWindow( g_hWnd_sm_auth_password_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_sm_resolve_domain_names, SW_HIDE );
	}
	else
	{
		_ShowWindow( g_hWnd_static_sm_hoz1, SW_SHOW );

		_ShowWindow( g_hWnd_sm_port_socks, SW_SHOW );

		_ShowWindow( g_hWnd_static_sm_port_socks, SW_SHOW );
		_ShowWindow( g_hWnd_static_sm_colon_socks, SW_SHOW );

		_ShowWindow( g_hWnd_chk_sm_type_hostname_socks, SW_SHOW );
		_ShowWindow( g_hWnd_chk_sm_type_ip_address_socks, SW_SHOW );

		if ( _SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		{
			_ShowWindow( g_hWnd_sm_ip_address_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_hostname_socks, SW_SHOW );
		}
		else
		{
			_ShowWindow( g_hWnd_sm_hostname_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_ip_address_socks, SW_SHOW );
		}

		if ( index == 3 )
		{
			_ShowWindow( g_hWnd_static_sm_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_static_sm_proxy_auth_password, SW_HIDE );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_password, SW_HIDE );

			_ShowWindow( g_hWnd_chk_sm_use_authentication_socks, SW_HIDE );

			_ShowWindow( g_hWnd_static_sm_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_static_sm_auth_password_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_password_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names, SW_HIDE );

			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname_ );

			_ShowWindow( g_hWnd_static_sm_auth_ident_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_sm_auth_ident_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names_v4a, SW_SHOW );
		}
		else if ( index == 4 )
		{
			_ShowWindow( g_hWnd_static_sm_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_static_sm_proxy_auth_password, SW_HIDE );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_password, SW_HIDE );

			_ShowWindow( g_hWnd_static_sm_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names_v4a, SW_HIDE );

			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

			_ShowWindow( g_hWnd_chk_sm_use_authentication_socks, SW_SHOW );

			_ShowWindow( g_hWnd_static_sm_auth_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_sm_auth_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_static_sm_auth_password_socks, SW_SHOW );
			_ShowWindow( g_hWnd_sm_auth_password_socks, SW_SHOW );

			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names, SW_SHOW );
		}
		else
		{
			_ShowWindow( g_hWnd_static_sm_proxy_auth_username, SW_SHOW );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_username, SW_SHOW );
			_ShowWindow( g_hWnd_static_sm_proxy_auth_password, SW_SHOW );
			_ShowWindow( g_hWnd_edit_sm_proxy_auth_password, SW_SHOW );

			_ShowWindow( g_hWnd_static_sm_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_ident_username_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names_v4a, SW_HIDE );

			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

			_ShowWindow( g_hWnd_chk_sm_use_authentication_socks, SW_HIDE );

			_ShowWindow( g_hWnd_static_sm_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_static_sm_auth_password_socks, SW_HIDE );
			_ShowWindow( g_hWnd_sm_auth_password_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_sm_resolve_domain_names, SW_HIDE );
		}
	}
}

LRESULT CALLBACK SMSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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
	}

	return _CallWindowProcW( SMProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK SMListSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_DIVIDERDBLCLICK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					int largest_width;

					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						largest_width = LVSCW_AUTOSIZE_USEHEADER;
					}
					else
					{
						// Need to scale each number for rounding purposes.
						largest_width = _SCALE_SM_( 5 ) + _SCALE_SM_( 16 ) + _SCALE_SM_( 5 );

						wchar_t tbuf[ 128 ];

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );

						int index = ( int )_SendMessageW( hWnd, LVM_GETTOPINDEX, 0, 0 );
						int index_end = ( int )_SendMessageW( hWnd, LVM_GETCOUNTPERPAGE, 0, 0 ) + index;

						RECT rc;
						HDC hDC = _GetDC( hWnd );
						HFONT ohf = ( HFONT )_SelectObject( hDC, hFont_site_manager );
						_DeleteObject( ohf );

						for ( ; index <= index_end; ++index )
						{
							lvi.iItem = index;
							lvi.mask = LVIF_PARAM;
							if ( _SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi ) == TRUE )
							{
								SITE_INFO *si = ( SITE_INFO * )lvi.lParam;
								if ( si != NULL )
								{
									wchar_t *buf = GetSiteInfoString( si, nmh->iItem, nmh->iItem + 1, tbuf, 128 );

									if ( buf == NULL )
									{
										tbuf[ 0 ] = L'\0';
										buf = tbuf;
									}

									rc.bottom = rc.left = rc.right = rc.top = 0;

									_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

									// Need to scale each number for rounding purposes.
									int width = ( rc.right - rc.left ) + _SCALE_SM_( 5 ) + _SCALE_SM_( 5 );	// 5 + 5 padding.
									if ( width > largest_width )
									{
										largest_width = width;
									}
								}
							}
							else
							{
								break;
							}
						}

						_ReleaseDC( hWnd, hDC );
					}

					_SendMessageW( hWnd, LVM_SETCOLUMNWIDTH, nmh->iItem, largest_width );

					return TRUE;
				}
				break;
			}
		}
		break;

		case WM_KEYDOWN:
		{
			switch ( wParam )
			{
				case VK_APPS:	// Context menu key.
				{
					int item_count = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
					int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM;
					lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, ( WPARAM )-1, LVNI_FOCUSED | LVNI_SELECTED );
					_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

					HWND hWnd_header = ( HWND )_SendMessageW( hWnd, LVM_GETHEADER, 0, 0 );

					RECT rc;
					_GetClientRect( hWnd, &rc );
					WINDOWPOS wp;
					_memzero( &wp, sizeof ( WINDOWPOS ) );
					HDLAYOUT hdl;
					hdl.prc = &rc;
					hdl.pwpos = &wp;
					_SendMessageW( hWnd_header, HDM_LAYOUT, 0, ( LPARAM )&hdl );

					POINT p;
					p.x = ( _SCALE_SM_( g_default_row_height ) / 2 );
					p.y = p.x + ( wp.cy - wp.y );

					if ( lvi.iItem != -1 )
					{
						rc.left = LVIR_BOUNDS;
						_SendMessageW( hWnd, LVM_GETITEMRECT, lvi.iItem, ( LPARAM )&rc );

						p.x = rc.left + ( ( rc.bottom - rc.top ) / 2 );
						p.y = rc.top + ( ( rc.bottom - rc.top ) / 2 );
					}

					_ClientToScreen( hWnd, &p );

					if ( lvi.iItem != -1 && lvi.lParam != NULL )
					{
						SITE_INFO *si = ( SITE_INFO * )lvi.lParam;

						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_ENABLE_SEL, ( si->enable ? MF_GRAYED : MF_ENABLED ) );
						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_DISABLE_SEL, ( si->enable ? MF_ENABLED : MF_GRAYED ) );
					}
					else
					{
						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_ENABLE_SEL, MF_GRAYED );
						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_DISABLE_SEL, MF_GRAYED );
					}

					_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_REMOVE_SEL, ( sel_count > 0 ? MF_ENABLED : MF_GRAYED ) );
					_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_SELECT_ALL, ( sel_count == item_count ? MF_GRAYED : MF_ENABLED ) );

					_TrackPopupMenu( g_hMenuSub_site_manager, 0, p.x, p.y, 0, _GetParent( hWnd ), NULL );
				}
			}
		}
		break;
	}

	return _CallWindowProcW( SMListProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK SMTabSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CTLCOLORSTATIC:
		{
			if ( g_sm_use_theme && _IsThemeActive() == TRUE )
			{
				if ( ( HWND )lParam == g_hWnd_btn_sm_authentication )
				{
					_SetBkMode( ( HDC )wParam, TRANSPARENT );

					if ( g_sm_draw_tab_pane )
					{
						POINT pt;
						pt.x = 0; pt.y = 0;

						_MapWindowPoints( hWnd, ( HWND )lParam, &pt, 1 );
						_SetBrushOrgEx( ( HDC )wParam, pt.x, pt.y, NULL );

						return ( INT_PTR )g_sm_tab_brush;
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

			_DeferWindowPos( hdwp, g_hWnd_btn_sm_authentication, HWND_TOP, _SCALE_SM_( 280 ), ( rc_tab.bottom - rc_tab.top ) + _SCALE_SM_( 65 ), _SCALE_SM_( 272 ), _SCALE_SM_( 72 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_site_manager;
		}
		break;
	}

	return _CallWindowProcW( SMTabProc, hWnd, msg, wParam, lParam );
}

void ShowHideSMTabs( int sw_type )
{
	int index = ( int )_SendMessageW( g_hWnd_sm_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
	if ( index != -1 )
	{
		switch ( index )
		{
			case 0:
			{
				_ShowWindow( g_hWnd_static_sm_category, sw_type );
				_ShowWindow( g_hWnd_sm_category, sw_type );

				_ShowWindow( g_hWnd_chk_sm_enable_download_directory, sw_type );
				_ShowWindow( g_hWnd_sm_download_directory, sw_type );
				_ShowWindow( g_hWnd_btn_sm_download_directory, sw_type );

				_ShowWindow( g_hWnd_chk_sm_enable_download_parts, sw_type );
				_ShowWindow( g_hWnd_sm_download_parts, sw_type );
				_ShowWindow( g_hWnd_ud_sm_download_parts, sw_type );

				_ShowWindow( g_hWnd_static_sm_ssl_version, sw_type );
				_ShowWindow( g_hWnd_sm_ssl_version, sw_type );

				_ShowWindow( g_hWnd_chk_sm_enable_speed_limit, sw_type );
				_ShowWindow( g_hWnd_sm_speed_limit, sw_type );

				_ShowWindow( g_hWnd_chk_sm_simulate_download, sw_type );
				_ShowWindow( g_hWnd_static_sm_download_operation, sw_type );
				_ShowWindow( g_hWnd_sm_download_operation, sw_type );

				_ShowWindow( g_hWnd_btn_sm_authentication, sw_type );
				_ShowWindow( g_hWnd_static_sm_username, sw_type );
				_ShowWindow( g_hWnd_edit_sm_username, sw_type );
				_ShowWindow( g_hWnd_static_sm_password, sw_type );
				_ShowWindow( g_hWnd_edit_sm_password, sw_type );
			}
			break;

			case 1:
			{
				_ShowWindow( g_hWnd_static_sm_comments, sw_type );
				_ShowWindow( g_hWnd_edit_sm_comments, sw_type );
			}
			break;

			case 2:
			{
				_ShowWindow( g_hWnd_static_sm_cookies, sw_type );
				_ShowWindow( g_hWnd_edit_sm_cookies, sw_type );
			}
			break;

			case 3:
			{
				_ShowWindow( g_hWnd_static_sm_headers, sw_type );
				_ShowWindow( g_hWnd_edit_sm_headers, sw_type );
			}
			break;

			case 4:
			{
				_ShowWindow( g_hWnd_chk_sm_send_data, sw_type );
				_ShowWindow( g_hWnd_edit_sm_data, sw_type );
			}
			break;

			case 5:
			{
				_ShowWindow( g_hWnd_static_sm_proxy_type, sw_type );
				_ShowWindow( g_hWnd_sm_proxy_type, sw_type );

				if ( sw_type == SW_SHOW )
				{
					index = ( int )_SendMessageW( g_hWnd_sm_proxy_type, CB_GETCURSEL, 0, 0 );

					if ( index == CB_ERR )
					{
						index = 0;
					}
				}
				else
				{
					index = 0;
				}
	
				ShowHideSMProxyWindows( index );
			}
			break;
		}
	}
}

void ResetSMTabPages()
{
	_SendMessageW( g_hWnd_edit_sm_site, WM_SETTEXT, 0, NULL );

	_SendMessageW( g_hWnd_sm_category, CB_SETCURSEL, 0, 0 );

	_SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_SETCHECK, BST_UNCHECKED, 0 );
	_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )cfg_default_download_directory );

	_EnableWindow( g_hWnd_chk_sm_enable_download_directory, TRUE );
	_EnableWindow( g_hWnd_sm_download_directory, FALSE );
	_EnableWindow( g_hWnd_btn_sm_download_directory, FALSE );

	_SendMessageW( g_hWnd_chk_sm_enable_download_parts, BM_SETCHECK, BST_UNCHECKED, 0 );
	_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );

	_EnableWindow( g_hWnd_sm_download_parts, FALSE );
	_EnableWindow( g_hWnd_ud_sm_download_parts, FALSE );

	_SendMessageW( g_hWnd_chk_sm_enable_speed_limit, BM_SETCHECK, BST_UNCHECKED, 0 );

	char value[ 21 ];
	_memzero( value, sizeof( char ) * 21 );
	__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
	_SendMessageA( g_hWnd_sm_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

	_EnableWindow( g_hWnd_sm_speed_limit, FALSE );


	_SendMessageW( g_hWnd_sm_ssl_version, CB_SETCURSEL, 0, 0 );

	_SendMessageW( g_hWnd_edit_sm_username, WM_SETTEXT, 0, NULL );
	_SendMessageW( g_hWnd_edit_sm_password, WM_SETTEXT, 0, NULL );

	_SendMessageW( g_hWnd_chk_sm_simulate_download, BM_SETCHECK, BST_UNCHECKED, 0 );
	_SendMessageW( g_hWnd_sm_download_operation, CB_SETCURSEL, 0, 0 );

	///////////

	_SendMessageA( g_hWnd_edit_sm_comments, WM_SETTEXT, 0, NULL );
	_SendMessageA( g_hWnd_edit_sm_cookies, WM_SETTEXT, 0, NULL );
	_SendMessageA( g_hWnd_edit_sm_headers, WM_SETTEXT, 0, NULL );
	_SendMessageA( g_hWnd_edit_sm_data, WM_SETTEXT, 0, NULL );

	_SendMessageW( g_hWnd_chk_sm_send_data, BM_SETCHECK, BST_UNCHECKED, 0 );
	_EnableWindow( g_hWnd_edit_sm_data, FALSE );

	///////////

	_SendMessageW( g_hWnd_sm_proxy_type, CB_SETCURSEL, 0, 0 );

	_SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
	_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );

	_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETTEXT, 0, NULL );
	_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_SETADDRESS, 0, NULL );
	_SendMessageW( g_hWnd_sm_port_socks, WM_SETTEXT, 0, NULL );

	_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_SETTEXT, 0, NULL );
	_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_SETTEXT, 0, NULL );

	_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_SETTEXT, 0, NULL );
	_SendMessageW( g_hWnd_chk_sm_resolve_domain_names_v4a, BM_SETCHECK, BST_UNCHECKED, 0 );

	_SendMessageW( g_hWnd_chk_sm_use_authentication_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
	_SendMessageW( g_hWnd_sm_auth_username_socks, WM_SETTEXT, 0, NULL );
	_SendMessageW( g_hWnd_sm_auth_password_socks, WM_SETTEXT, 0, NULL );
	_EnableWindow( g_hWnd_static_sm_auth_username_socks, FALSE );
	_EnableWindow( g_hWnd_sm_auth_username_socks, FALSE );
	_EnableWindow( g_hWnd_static_sm_auth_password_socks, FALSE );
	_EnableWindow( g_hWnd_sm_auth_password_socks, FALSE );
	_SendMessageW( g_hWnd_chk_sm_resolve_domain_names, BM_SETCHECK, BST_UNCHECKED, 0 );

	if ( _SendMessageW( g_hWnd_sm_tab, TCM_GETCURSEL, 0, 0 ) == 4 )
	{
		ShowHideSMProxyWindows( 0 );
	}
}

// Sort function for columns.
int CALLBACK LMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	SORT_INFO *si = ( SORT_INFO * )lParamSort;

	if ( si->hWnd == g_hWnd_site_list )
	{
		SITE_INFO *si1 = ( SITE_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		SITE_INFO *si2 = ( SITE_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		switch ( si->column )
		{
			case SM_COLUMN_SITE: { return lstrcmpW( si1->w_host, si2->w_host ); } break;
			case SM_COLUMN_CATEGORY: { return lstrcmpW( si1->category, si2->category ); } break;
			case SM_COLUMN_DOWNLOAD_DIRECTORY: { return _wcsicmp_s( si1->download_directory, si2->download_directory ); } break;

			case SM_COLUMN_DOWNLOAD_PARTS: { return ( si1->parts > si2->parts ); } break;
			case SM_COLUMN_DOWNLOAD_SPEED_LIMIT: { return ( si1->download_speed_limit > si2->download_speed_limit ); } break;

			case SM_COLUMN_SSL_TLS_VERSION: { return ( si1->ssl_version > si2->ssl_version ); } break;

			case SM_COLUMN_USERNAME: { return lstrcmpW( si1->w_username, si2->w_username ); } break;
			case SM_COLUMN_PASSWORD:
			{
				if ( g_show_passwords )
				{
					return lstrcmpW( si1->w_password, si2->w_password );
				}
			}
			break;

			case SM_COLUMN_DOWNLOAD_OPERATIONS:
			{
				if ( si1->download_operations & DOWNLOAD_OPERATION_SIMULATE && !( si2->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
				{
					return 1;
				}
				else if ( si2->download_operations & DOWNLOAD_OPERATION_SIMULATE && !( si1->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
				{
					return -1;
				}
				else
				{
					return ( si1->download_operations > si2->download_operations );
				}
			}
			break;

			case SM_COLUMN_COMMENTS: { return _wcsicmp_s( si1->comments, si2->comments ); } break;
			case SM_COLUMN_COOKIES: { return _stricmp_s( si1->utf8_cookies, si2->utf8_cookies ); } break;
			case SM_COLUMN_HEADERS: { return _stricmp_s( si1->utf8_headers, si2->utf8_headers ); } break;
			case SM_COLUMN_POST_DATA: { return _stricmp_s( si1->utf8_data, si2->utf8_data ); } break;

			case SM_COLUMN_PROXY_TYPE: { return ( si1->proxy_info.type > si2->proxy_info.type ); } break;

			case SM_COLUMN_PROXY_SERVER:
			{
				if ( si1->proxy_info.address_type == 0 && si2->proxy_info.address_type == 0 )
				{
					return lstrcmpW( si1->proxy_info.hostname, si2->proxy_info.hostname );
				}
				else if ( si1->proxy_info.address_type == 0 )
				{
					if ( si1->proxy_info.hostname == NULL )
					{
						return -1;
					}
					else
					{
						return 1;
					}
				}
				else if ( si2->proxy_info.address_type == 0 )
				{
					if ( si2->proxy_info.hostname == NULL )
					{
						return 1;
					}
					else
					{
						return -1;
					}
				}
				else
				{
					return ( si1->proxy_info.ip_address > si2->proxy_info.ip_address );
				}
			}
			break;

			case SM_COLUMN_PROXY_PORT: { return ( si1->proxy_info.port > si2->proxy_info.port ); } break;

			case SM_COLUMN_PROXY_USERNAME: { return lstrcmpW( si1->proxy_info.w_username, si2->proxy_info.w_username ); } break;
			case SM_COLUMN_PROXY_PASSWORD:
			{
				if ( g_show_passwords )
				{
					return lstrcmpW( si1->proxy_info.w_password, si2->proxy_info.w_password );
				}
			}
			break;

			case SM_COLUMN_RESOLVE_DOMAIN_NAMES:
			{
				if ( si1->proxy_info.resolve_domain_names == si2->proxy_info.resolve_domain_names )
				{
					return 0;
				}
				else if ( si1->proxy_info.resolve_domain_names && !si2->proxy_info.resolve_domain_names )
				{
					return 1;
				}
				else
				{
					return -1;
				}
			}

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

void SelectSiteItem( int index )
{
	BOOL enable;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.iItem = index;
	lvi.mask = LVIF_PARAM | LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	_SendMessageW( g_hWnd_site_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

	SITE_INFO *si = ( SITE_INFO * )lvi.lParam;
	if ( si != NULL && ( lvi.state & LVIS_SELECTED ) )
	{
		if ( si == g_selected_site_info )
		{
			return;
		}

		g_selected_site_info = si;
		g_selected_site_index = index;

		_SendMessageW( g_hWnd_edit_sm_site, WM_SETTEXT, 0, ( LPARAM )si->w_host );

		int sel_index = 0;
		if ( si->category != NULL )
		{
			sel_index = ( int )_SendMessageW( g_hWnd_sm_category, CB_FINDSTRINGEXACT, 0, ( LPARAM )si->category );
			if ( sel_index < 0 )
			{
				sel_index = 0;
			}
		}
		_SendMessageW( g_hWnd_sm_category, CB_SETCURSEL, sel_index, 0 );

		_SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_SETCHECK, ( si->use_download_directory ? BST_CHECKED : BST_UNCHECKED ), 0 );

		if ( si->download_operations & DOWNLOAD_OPERATION_SIMULATE )
		{
			_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )ST_V__Simulated_ );

			_EnableWindow( g_hWnd_chk_sm_enable_download_directory, FALSE );

			enable = FALSE;
		}
		else
		{
			_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )( si->download_directory != NULL ? si->download_directory : cfg_default_download_directory ) );

			_EnableWindow( g_hWnd_chk_sm_enable_download_directory, TRUE );

			enable = ( si->use_download_directory ? TRUE : FALSE );
		}

		_EnableWindow( g_hWnd_sm_download_directory, enable );
		_EnableWindow( g_hWnd_btn_sm_download_directory, enable );

		_SendMessageW( g_hWnd_chk_sm_enable_download_parts, BM_SETCHECK, ( si->use_parts ? BST_CHECKED : BST_UNCHECKED ), 0 );
		_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETPOS, 0, ( si->use_parts && si->parts > 0 ? si->parts : cfg_default_download_parts ) );

		enable = ( si->use_parts ? TRUE : FALSE );

		_EnableWindow( g_hWnd_sm_download_parts, enable );
		_EnableWindow( g_hWnd_ud_sm_download_parts, enable );

		_SendMessageW( g_hWnd_chk_sm_enable_speed_limit, BM_SETCHECK, ( si->use_download_speed_limit ? BST_CHECKED : BST_UNCHECKED ), 0 );

		char value[ 21 ];
		_memzero( value, sizeof( char ) * 21 );
		__snprintf( value, 21, "%I64u", ( si->use_download_speed_limit ? si->download_speed_limit : cfg_default_speed_limit ) );
		_SendMessageA( g_hWnd_sm_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

		_EnableWindow( g_hWnd_sm_speed_limit, ( si->use_download_speed_limit ? TRUE : FALSE ) );


		_SendMessageW( g_hWnd_sm_ssl_version, CB_SETCURSEL, si->ssl_version, 0 );

		_SendMessageW( g_hWnd_edit_sm_username, WM_SETTEXT, 0, ( LPARAM )si->w_username );
		_SendMessageW( g_hWnd_edit_sm_password, WM_SETTEXT, 0, ( LPARAM )si->w_password );


		_SendMessageW( g_hWnd_chk_sm_simulate_download, BM_SETCHECK, ( si->download_operations & DOWNLOAD_OPERATION_SIMULATE ? BST_CHECKED : BST_UNCHECKED ), 0 );
		unsigned char download_operation_index;
		if ( si->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) { download_operation_index = 1; }
		else if ( si->download_operations & DOWNLOAD_OPERATION_VERIFY ) { download_operation_index = 2; }
		else															{ download_operation_index = 0; }
		_SendMessageW( g_hWnd_sm_download_operation, CB_SETCURSEL, download_operation_index, 0 );

		//////////

		_SendMessageW( g_hWnd_edit_sm_comments, WM_SETTEXT, 0, ( LPARAM )si->comments );
		_SendMessageA( g_hWnd_edit_sm_cookies, WM_SETTEXT, 0, ( LPARAM )si->utf8_cookies );
		_SendMessageA( g_hWnd_edit_sm_headers, WM_SETTEXT, 0, ( LPARAM )si->utf8_headers );
		_SendMessageA( g_hWnd_edit_sm_data, WM_SETTEXT, 0, ( LPARAM )si->utf8_data );

		_SendMessageW( g_hWnd_chk_sm_send_data, BM_SETCHECK, ( si->method == METHOD_POST ? BST_CHECKED : BST_UNCHECKED ), 0 );
		_EnableWindow( g_hWnd_edit_sm_data, ( si->method == METHOD_POST ? TRUE : FALSE ) );


		//////////

		//	RESET

		_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_SETADDRESS, 0, NULL );
		_SendMessageW( g_hWnd_sm_port_socks, WM_SETTEXT, 0, NULL );

		_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_SETTEXT, 0, NULL );

		_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_chk_sm_resolve_domain_names_v4a, BM_SETCHECK, BST_UNCHECKED, 0 );

		_SendMessageW( g_hWnd_chk_sm_use_authentication_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_sm_auth_username_socks, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_sm_auth_password_socks, WM_SETTEXT, 0, NULL );
		_EnableWindow( g_hWnd_static_sm_auth_username_socks, FALSE );
		_EnableWindow( g_hWnd_sm_auth_username_socks, FALSE );
		_EnableWindow( g_hWnd_static_sm_auth_password_socks, FALSE );
		_EnableWindow( g_hWnd_sm_auth_password_socks, FALSE );
		_SendMessageW( g_hWnd_chk_sm_resolve_domain_names, BM_SETCHECK, BST_UNCHECKED, 0 );

		//

		_SendMessageW( g_hWnd_sm_proxy_type, CB_SETCURSEL, si->proxy_info.type, 0 );

		if ( si->proxy_info.address_type == 0 )
		{
			_SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );
		}
		else
		{
			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
			_SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, BM_SETCHECK, BST_CHECKED, 0 );
		}

		if ( si->proxy_info.type != 0 )
		{
			if ( si->proxy_info.address_type == 0 )
			{
				_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.hostname );
				_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_SETADDRESS, 0, 2130706433 );	// 127.0.0.1
			}
			else
			{
				_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
				_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_SETADDRESS, 0, ( LPARAM )si->proxy_info.ip_address );
			}

			__snprintf( value, 6, "%hu", si->proxy_info.port );
			_SendMessageA( g_hWnd_sm_port_socks, WM_SETTEXT, 0, ( LPARAM )value );

			if ( si->proxy_info.type == 1 || si->proxy_info.type == 2 )	// HTTP and HTTPS
			{
				_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.w_username );
				_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.w_password );
			}
			else if ( si->proxy_info.type == 3 )	// SOCKS v4
			{
				_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.w_username );

				_SendMessageW( g_hWnd_chk_sm_resolve_domain_names_v4a, BM_SETCHECK, ( si->proxy_info.resolve_domain_names ? BST_CHECKED : BST_UNCHECKED ), 0 );
			}
			else if ( si->proxy_info.type == 4 )	// SOCKS v5
			{
				_SendMessageW( g_hWnd_chk_sm_use_authentication_socks, BM_SETCHECK, ( si->proxy_info.use_authentication ? BST_CHECKED : BST_UNCHECKED ), 0 );

				if ( si->proxy_info.use_authentication )
				{
					_SendMessageW( g_hWnd_sm_auth_username_socks, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.w_username );
					_SendMessageW( g_hWnd_sm_auth_password_socks, WM_SETTEXT, 0, ( LPARAM )si->proxy_info.w_password );

					enable = TRUE;
				}
				else
				{
					enable = FALSE;
				}

				_EnableWindow( g_hWnd_static_sm_auth_username_socks, enable );
				_EnableWindow( g_hWnd_sm_auth_username_socks, enable );
				_EnableWindow( g_hWnd_static_sm_auth_password_socks, enable );
				_EnableWindow( g_hWnd_sm_auth_password_socks, enable );

				_SendMessageW( g_hWnd_chk_sm_resolve_domain_names, BM_SETCHECK, ( si->proxy_info.resolve_domain_names ? BST_CHECKED : BST_UNCHECKED ), 0 );
			}
		}

		if ( _SendMessageW( g_hWnd_sm_tab, TCM_GETCURSEL, 0, 0 ) == 4 )
		{
			ShowHideSMProxyWindows( si->proxy_info.type );
		}
	}
	else
	{
		g_selected_site_info = NULL;
		g_selected_site_index = -1;

		_EnableWindow( g_hWnd_remove_site, FALSE );

		ResetSMTabPages();
	}
}

LRESULT CALLBACK SiteManagerWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			current_dpi_site_manager = __GetDpiForWindow( hWnd );
			last_dpi_site_manager = ( current_dpi_site_manager == current_dpi_main ? current_dpi_site_manager : 0 );
			hFont_site_manager = UpdateFont( current_dpi_site_manager );

			// If the main window is WS_EX_COMPOSITED, then this flickers like mad.
			g_hWnd_site_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_site_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );



			// Give this a height and width so the tabs show up.
			g_hWnd_sm_tab = _CreateWindowW( /*| WS_EX_CONTROLPARENT,*/ WC_TABCONTROL, NULL, WS_CHILD | /*WS_CLIPCHILDREN |*/ WS_TABSTOP | WS_VISIBLE, 0, 0, MIN_WIDTH - 20, 250, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_V_General;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Comments;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Cookies;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Headers;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 3, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_POST_Data;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 4, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Proxy;
			_SendMessageW( g_hWnd_sm_tab, TCM_INSERTITEM, 5, ( LPARAM )&ti );	// Insert a new tab at the end.


			g_hWnd_static_sm_category = _CreateWindowW( WC_STATIC, ST_V_Category_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_sm_category = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_SM_CATEGORY, NULL, NULL );

			_SendMessageW( g_hWnd_sm_category, CB_SETCURSEL, 0, 0 );

			g_hWnd_chk_sm_enable_download_directory = _CreateWindowW( WC_BUTTON, ST_V_Download_directory_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SM_ENABLE_DOWNLOAD_DIRECTORY, NULL, NULL );
			g_hWnd_sm_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_sm_download_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_chk_sm_enable_download_parts = _CreateWindowW( WC_BUTTON, ST_V_Download_parts_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SM_ENABLE_DOWNLOAD_PARTS, NULL, NULL );
			// Needs dimensions so that the spinner control can size itself.
			g_hWnd_sm_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 100, 23, hWnd, ( HMENU )EDIT_SM_DOWNLOAD_PARTS, NULL, NULL );

			g_hWnd_ud_sm_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_sm_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_sm_download_parts, 0 );
			_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_ud_sm_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );

			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_sm_download_parts, &rc_spinner );
			site_manager_spinner_width = rc_spinner.right - rc_spinner.left;
			site_manager_spinner_height = rc_spinner.bottom - rc_spinner.top;


			g_hWnd_chk_sm_enable_speed_limit = _CreateWindowW( WC_BUTTON, ST_V_Download_speed_limit_bytes_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SM_ENABLE_SPEED_LIMIT, NULL, NULL );
			g_hWnd_sm_speed_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SM_SPEED_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_sm_speed_limit, EM_LIMITTEXT, 20, 0 );


			g_hWnd_sm_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			sm_limit_tooltip_text[ 0 ] = 0;

			TOOLINFO tti;
			_memzero( &tti, sizeof( TOOLINFO ) );
			tti.cbSize = sizeof( TOOLINFO );
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = g_hWnd_sm_speed_limit;
			tti.lpszText = sm_limit_tooltip_text;

			_GetClientRect( hWnd, &tti.rect );
			_SendMessageW( g_hWnd_sm_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_sm_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );





			g_hWnd_static_sm_ssl_version = _CreateWindowW( WC_STATIC, ST_V_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_sm_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_Default );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );
			if ( g_can_use_tls_1_3 )
			{
				_SendMessageW( g_hWnd_sm_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_3 );
			}

			_SendMessageW( g_hWnd_sm_ssl_version, CB_SETCURSEL, 0, 0 );








			// Doesn't draw properly in XP if it's not a child of the tab control.
			g_hWnd_btn_sm_authentication = _CreateWindowW( WC_BUTTON, ST_V_Authentication, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, g_hWnd_sm_tab, NULL, NULL, NULL );

			g_hWnd_static_sm_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_USERNAME_SITE, NULL, NULL );

			g_hWnd_static_sm_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_PASSWORD_SITE, NULL, NULL );



			g_hWnd_chk_sm_simulate_download = _CreateWindowW( WC_BUTTON, ST_V_Simulate_download, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SM_SIMULATE_DOWNLOAD, NULL, NULL );
			
			g_hWnd_static_sm_download_operation = _CreateWindowW( WC_STATIC, ST_V_Download_operation_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_sm_download_operation = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_SM_DOWNLOAD_OPERATION, NULL, NULL );
			_SendMessageW( g_hWnd_sm_download_operation, CB_ADDSTRING, 0, ( LPARAM )ST_V_Download );
			_SendMessageW( g_hWnd_sm_download_operation, CB_ADDSTRING, 0, ( LPARAM )ST_V_Add );
			_SendMessageW( g_hWnd_sm_download_operation, CB_ADDSTRING, 0, ( LPARAM )ST_V_Verify );

			_SendMessageW( g_hWnd_sm_download_operation, CB_SETCURSEL, 0, 0 );




			///////////////////////////////////

			g_hWnd_static_sm_comments = _CreateWindowW( WC_STATIC, ST_V_Comments_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_comments = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_sm_cookies = _CreateWindowW( WC_STATIC, ST_V_Cookies_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_cookies = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_sm_headers = _CreateWindowW( WC_STATIC, ST_V_Headers_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_headers = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_sm_send_data = _CreateWindowW( WC_BUTTON, ST_V_Send_POST_Data_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SM_SEND_DATA, NULL, NULL );
			g_hWnd_edit_sm_data = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );


			///////////////////////////////////


			g_hWnd_static_sm_proxy_type = _CreateWindowW( WC_STATIC, ST_V_Use_proxy_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_sm_proxy_type = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_SM_PROXY_TYPE, NULL, NULL );
			_SendMessageW( g_hWnd_sm_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_Default );
			_SendMessageW( g_hWnd_sm_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_HTTP );
			_SendMessageW( g_hWnd_sm_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_HTTPS );
			_SendMessageW( g_hWnd_sm_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_SOCKS_v4 );
			_SendMessageW( g_hWnd_sm_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_SOCKS_v5 );

			_SendMessageW( g_hWnd_sm_proxy_type, CB_SETCURSEL, 0, 0 );


			g_hWnd_static_sm_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_sm_type_hostname_socks = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_TYPE_HOST_SOCKS, NULL, NULL );
			g_hWnd_chk_sm_type_ip_address_socks = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_TYPE_IP_ADDRESS_SOCKS, NULL, NULL );

			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );

			g_hWnd_sm_hostname_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SM_HOST_SOCKS, NULL, NULL );
			// Needs a width and height when it's created because it's a stupid control.
			g_hWnd_sm_ip_address_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 0, 310, 23, hWnd, ( HMENU )EDIT_SM_IP_ADDRESS_SOCKS, NULL, NULL );


			g_hWnd_static_sm_colon_socks = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_sm_port_socks = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_sm_port_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SM_PORT_SOCKS, NULL, NULL );


			g_hWnd_static_sm_proxy_auth_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_proxy_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_SM_PROXY_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_sm_proxy_auth_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_proxy_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_SM_PROXY_AUTH_PASSWORD, NULL, NULL );


			// v4

			g_hWnd_static_sm_auth_ident_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_sm_auth_ident_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_SM_AUTH_IDENT_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_chk_sm_resolve_domain_names_v4a = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names_v4a, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_RESOLVE_DOMAIN_NAMES_V4A, NULL, NULL );

			// v5

			g_hWnd_chk_sm_use_authentication_socks = _CreateWindowW( WC_BUTTON, ST_V_Use_Authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_AUTHENTICATION_SOCKS, NULL, NULL );

			g_hWnd_static_sm_auth_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_sm_auth_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_SM_AUTH_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_static_sm_auth_password_socks = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_sm_auth_password_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_SM_AUTH_PASSWORD_SOCKS, NULL, NULL );

			g_hWnd_chk_sm_resolve_domain_names = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SM_RESOLVE_DOMAIN_NAMES, NULL, NULL );




			_SendMessageW( g_hWnd_sm_hostname_socks, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_sm_port_socks, EM_LIMITTEXT, 5, 0 );

			_SendMessageW( g_hWnd_sm_auth_username_socks, EM_LIMITTEXT, 255, 0 );
			_SendMessageW( g_hWnd_sm_auth_password_socks, EM_LIMITTEXT, 255, 0 );








			///////////////////////////////////


			g_hWnd_static_sm_site = _CreateWindowW( WC_STATIC, ST_V_Site_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sm_site = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SITE, NULL, NULL );


			g_hWnd_new_site = _CreateWindowW( WC_BUTTON, ST_V_New, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_NEW_SITE, NULL, NULL );
			g_hWnd_save_site = _CreateWindowW( WC_BUTTON, ST_V_Save, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SAVE_SITE, NULL, NULL );
			g_hWnd_remove_site = _CreateWindowW( WC_BUTTON, ST_V_Remove, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_REMOVE_SITE, NULL, NULL );

			g_hWnd_chk_show_passwords = _CreateWindowW( WC_BUTTON, ST_V_Show_passwords, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SHOW_PASSWORDS, NULL, NULL );
			g_hWnd_close_lm_wnd = _CreateWindowW( WC_BUTTON, ST_V_Close, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CLOSE_SITE_MANAGER_WND, NULL, NULL );

			_SendMessageW( g_hWnd_edit_sm_site, EM_LIMITTEXT, MAX_DOMAIN_LENGTH + 8, 0 );	// Include "https://"

			g_default_pw_char = ( wchar_t )_SendMessageW( g_hWnd_edit_sm_password, EM_GETPASSWORDCHAR, 0, 0 );

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.pszText = ST_V_NUM;
			lvc.cx = 35;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Site;
			lvc.cx = 240;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Category;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 2, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Download_Directory;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 3, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Download_Parts;
			lvc.cx = 140;
			lvc.fmt = LVCFMT_RIGHT;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 4, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Download_Speed_Limit;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 5, ( LPARAM )&lvc );

			lvc.pszText = ST_V_SSL___TLS_Version;
			lvc.cx = 140;
			lvc.fmt = LVCFMT_LEFT;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 6, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Username;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 7, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Password;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 8, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Download_Operations;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 9, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Comments;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 10, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Cookies;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 11, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Headers;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 12, ( LPARAM )&lvc );

			lvc.pszText = ST_V_POST_Data;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 13, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Proxy_Type;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 14, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Proxy_Server;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 15, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Proxy_Port;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 16, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Proxy_Username;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 17, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Proxy_Password;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 18, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Resolve_Domain_Names;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTCOLUMN, 19, ( LPARAM )&lvc );



			g_hMenuSub_site_manager = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Enable;
			mii.cch = ST_L_Enable;
			mii.wID = MENU_SM_ENABLE_SEL;
			_InsertMenuItemW( g_hMenuSub_site_manager, 0, TRUE, &mii );

			mii.dwTypeData = ST_V_Disable;
			mii.cch = ST_L_Disable;
			mii.wID = MENU_SM_DISABLE_SEL;
			_InsertMenuItemW( g_hMenuSub_site_manager, 1, TRUE, &mii );

			mii.fType = MFT_SEPARATOR;
			_InsertMenuItemW( g_hMenuSub_site_manager, 2, TRUE, &mii );

			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Remove;
			mii.cch = ST_L_Remove;
			mii.wID = MENU_SM_REMOVE_SEL;
			_InsertMenuItemW( g_hMenuSub_site_manager, 3, TRUE, &mii );

			mii.fType = MFT_SEPARATOR;
			_InsertMenuItemW( g_hMenuSub_site_manager, 4, TRUE, &mii );

			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Select_All;
			mii.cch = ST_L_Select_All;
			mii.wID = MENU_SM_SELECT_ALL;
			_InsertMenuItemW( g_hMenuSub_site_manager, 5, TRUE, &mii );

			//_SetFocus( g_hWnd_edit_sm_site );

			_SendMessageW( g_hWnd_site_list, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_sm_tab, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_category, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_category, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_enable_download_directory, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_download_directory, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_btn_sm_download_directory, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_enable_download_parts, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_download_parts, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_ssl_version, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_ssl_version, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_enable_speed_limit, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_speed_limit, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_comments, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_comments, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_cookies, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_cookies, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_headers, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_headers, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_chk_sm_send_data, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_data, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			//

			_SendMessageW( g_hWnd_static_sm_proxy_type, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_proxy_type, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_type_hostname_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_colon_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_port_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_port_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );



			_SendMessageW( g_hWnd_static_sm_proxy_auth_username, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_proxy_auth_password, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );


			_SendMessageW( g_hWnd_static_sm_auth_ident_username_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_resolve_domain_names_v4a, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_use_authentication_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_auth_username_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_auth_username_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_static_sm_auth_password_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_auth_password_socks, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_resolve_domain_names, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			//


			_SendMessageW( g_hWnd_static_sm_site, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_site, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_btn_sm_authentication, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_username, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_username, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_password, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_edit_sm_password, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_sm_simulate_download, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_static_sm_download_operation, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_sm_download_operation, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_new_site, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_save_site, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_remove_site, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );

			_SendMessageW( g_hWnd_chk_show_passwords, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );
			_SendMessageW( g_hWnd_close_lm_wnd, WM_SETFONT, ( WPARAM )hFont_site_manager, 0 );


			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			hFont_copy_sm_proxy = UpdateFont( current_dpi_site_manager );
			_SendMessageW( g_hWnd_sm_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_copy_sm_proxy, 0 );


			t_sm_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
			if ( t_sm_download_directory != NULL )
			{
				_wmemcpy_s( t_sm_download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
				t_sm_download_directory[ g_default_download_directory_length ] = 0;	// Sanity.
			}

			SMListProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_site_list, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_site_list, GWLP_WNDPROC, ( LONG_PTR )SMListSubProc );

			SMProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_edit_sm_comments, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_edit_sm_comments, GWLP_WNDPROC, ( LONG_PTR )SMSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_sm_cookies, GWLP_WNDPROC, ( LONG_PTR )SMSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_sm_headers, GWLP_WNDPROC, ( LONG_PTR )SMSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_sm_data, GWLP_WNDPROC, ( LONG_PTR )SMSubProc );

			// g_hWnd_btn_sm_authentication doesn't draw properly in XP if it's not a child of the tab control.
			SMTabProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_sm_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_sm_tab, GWLP_WNDPROC, ( LONG_PTR )SMTabSubProc );

			#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					g_sm_use_theme = InitializeUXTheme();
				}
			#endif

			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, load_site_list, ( void * )NULL, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}

			int width = _SCALE_SM_( MIN_WIDTH );
			int height = _SCALE_SM_( MIN_HEIGHT );

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

			g_sm_draw_tab_pane = !IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case CB_SM_CATEGORY:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						BOOL enable;

						int cur_sel = ( int )_SendMessageW( g_hWnd_sm_category, CB_GETCURSEL, 0, 0 );
						if ( cur_sel == 0 )
						{
							enable = FALSE;

							GlobalFree( t_sm_download_directory );
							t_sm_download_directory = GlobalStrDupW( cfg_default_download_directory );
						}
						else
						{
							enable = TRUE;

							LRESULT ret = _SendMessageW( g_hWnd_sm_category, CB_GETITEMDATA, cur_sel, 0 );
							if ( ret != CB_ERR )
							{
								CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )ret;
								if ( ci != NULL )
								{
									GlobalFree( t_sm_download_directory );
									t_sm_download_directory = GlobalStrDupW( ci->download_directory );
								}
							}
						}

						if ( _SendMessageW( g_hWnd_chk_sm_simulate_download, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED )
						{
							_EnableWindow( g_hWnd_chk_sm_enable_download_directory, TRUE );

							_EnableWindow( g_hWnd_sm_download_directory, enable );
							_EnableWindow( g_hWnd_btn_sm_download_directory, enable );

							_SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_SETCHECK, ( enable == TRUE ? BST_CHECKED : BST_UNCHECKED ), 0 );

							_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )t_sm_download_directory );
						}
					}
				}
				break;

				case CHK_SM_ENABLE_DOWNLOAD_DIRECTORY:
				case BTN_SM_DOWNLOAD_DIRECTORY:
				{
					if ( LOWORD( wParam ) == CHK_SM_ENABLE_DOWNLOAD_DIRECTORY )
					{
						BOOL enable = ( _SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

						_EnableWindow( g_hWnd_sm_download_directory, enable );
						_EnableWindow( g_hWnd_btn_sm_download_directory, enable );

						if ( enable == FALSE )
						{
							break;
						}
					}

					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_sm_download_directory );
						t_sm_download_directory = directory;

						_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )t_sm_download_directory );
					}
				}
				break;

				case CHK_SM_ENABLE_DOWNLOAD_PARTS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_sm_enable_download_parts, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_sm_download_parts, enable );
					_EnableWindow( g_hWnd_ud_sm_download_parts, enable );
				}
				break;

				case EDIT_SM_DOWNLOAD_PARTS:
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

				case CHK_SM_ENABLE_SPEED_LIMIT:
				{
					_EnableWindow( g_hWnd_sm_speed_limit, ( _SendMessageW( g_hWnd_chk_sm_enable_speed_limit, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
				}
				break;

				case EDIT_SM_SPEED_LIMIT:
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
							unsigned int length = FormatSizes( sm_limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
							sm_limit_tooltip_text[ length++ ] = L'/';
							sm_limit_tooltip_text[ length++ ] = L's';
							sm_limit_tooltip_text[ length ] = 0;
						}
						else
						{
							_wmemcpy_s( sm_limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_sm_speed_limit;
						ti.lpszText = sm_limit_tooltip_text;
						_SendMessageW( g_hWnd_sm_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );
					}
				}
				break;

				case CHK_SM_SIMULATE_DOWNLOAD:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_sm_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? FALSE : TRUE );

					_EnableWindow( g_hWnd_chk_sm_enable_download_directory, enable );

					_SendMessageW( g_hWnd_sm_download_directory, WM_SETTEXT, 0, ( LPARAM )( enable == FALSE ? ST_V__Simulated_ : t_sm_download_directory ) );

					if ( enable == TRUE && _SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED )
					{
						enable = FALSE;
					}

					_EnableWindow( g_hWnd_sm_download_directory, enable );
					_EnableWindow( g_hWnd_btn_sm_download_directory, enable );
				}
				break;

				case CHK_SM_SEND_DATA:
				{
					_EnableWindow( g_hWnd_edit_sm_data, ( _SendMessageW( g_hWnd_chk_sm_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
				}
				break;

				case CB_SM_PROXY_TYPE:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_sm_proxy_type, CB_GETCURSEL, 0, 0 );

						if ( index != CB_ERR )
						{
							_SendMessageW( g_hWnd_sm_hostname_socks, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
							_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_SETADDRESS, 0, 2130706433 );	// 127.0.0.1

							char value[ 6 ];
							_memzero( value, sizeof( char ) * 6 );
							__snprintf( value, 6, "%hu", ( index == 1 ? 80 : ( index == 2 ? 443 : 1080 ) ) );
							_SendMessageA( g_hWnd_sm_port_socks, WM_SETTEXT, 0, ( LPARAM )value );

							ShowHideSMProxyWindows( index );
						}
					}
				}
				break;

				case BTN_SM_AUTHENTICATION_SOCKS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_sm_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_static_sm_auth_username_socks, enable );
					_EnableWindow( g_hWnd_sm_auth_username_socks, enable );
					_EnableWindow( g_hWnd_static_sm_auth_password_socks, enable );
					_EnableWindow( g_hWnd_sm_auth_password_socks, enable );
				}
				break;

				case EDIT_SM_PORT_SOCKS:
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

				case BTN_SM_TYPE_HOST_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_sm_type_hostname_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_sm_ip_address_socks, SW_HIDE );
						_ShowWindow( g_hWnd_sm_hostname_socks, SW_SHOW );
					}
				}
				break;

				case BTN_SM_TYPE_IP_ADDRESS_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_sm_hostname_socks, SW_HIDE );
						_ShowWindow( g_hWnd_sm_ip_address_socks, SW_SHOW );
					}
				}
				break;

				case BTN_NEW_SITE:
				{
					g_selected_site_info = NULL;
					g_selected_site_index = -1;

					ResetSMTabPages();

					// Set the state of all items to deselected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_site_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );

					_SetFocus( g_hWnd_edit_sm_site );
				}
				break;

				case BTN_SAVE_SITE:
				{
					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_site, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 )
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.iItem = g_selected_site_index;
						lvi.mask = LVIF_PARAM | LVIF_STATE;
						lvi.stateMask = LVIS_SELECTED;
						_SendMessageW( g_hWnd_site_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

						SITE_INFO *si = ( SITE_INFO * )lvi.lParam;
						if ( si != NULL && ( lvi.state & LVIS_SELECTED ) )
						{
							g_selected_site_info = si;
						}
						else
						{
							g_selected_site_info = NULL;
							g_selected_site_index = -1;
						}

						SITE_UPDATE_INFO *sui = ( SITE_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SITE_UPDATE_INFO ) );
						if ( sui != NULL )
						{
							sui->update_type = 0;	// Add
							sui->old_si = g_selected_site_info;
							sui->index = g_selected_site_index;

							si = ( SITE_INFO * )GlobalAlloc( GPTR, sizeof( SITE_INFO ) );
							if ( si != NULL )
							{
								unsigned int download_operations = ( _SendMessageW( g_hWnd_chk_sm_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? DOWNLOAD_OPERATION_SIMULATE : DOWNLOAD_OPERATION_NONE );

								si->enable = true;

								si->w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );	// Include the NULL terminator.
								_SendMessageW( g_hWnd_edit_sm_site, WM_GETTEXT, edit_length + 1, ( LPARAM )si->w_host );

								// Make sure the domain doesn't exceed 253 characters for HTTP and HTTPS.
								// Check to see if it's not an HTTPS site. If it isn't then an extra character could have been added.
								if ( edit_length >= ( MAX_DOMAIN_LENGTH + 8 ) && si->w_host[ 4 ] == ':' )
								{
									si->w_host[ edit_length - 1 ] = 0;	// Sanity.
								}


								// GENERAL TAB


								int cur_sel = ( int )_SendMessageW( g_hWnd_sm_category, CB_GETCURSEL, 0, 0 );
								LRESULT ret = _SendMessageW( g_hWnd_sm_category, CB_GETITEMDATA, cur_sel, 0 );
								if ( ret != CB_ERR )
								{
									CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )ret;
									if ( ci != NULL )
									{
										si->category = GlobalStrDupW( ci->category );
									}
								}

								if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
								{
									si->use_download_directory = ( _SendMessageW( g_hWnd_chk_sm_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

									if ( si->use_download_directory )
									{
										edit_length = ( unsigned int )_SendMessageW( g_hWnd_sm_download_directory, WM_GETTEXTLENGTH, 0, 0 );
										if ( edit_length > 0 )
										{
											++edit_length; // Include the NULL terminator.
											si->download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * edit_length );
											_SendMessageW( g_hWnd_sm_download_directory, WM_GETTEXT, edit_length, ( LPARAM )si->download_directory );
										}
										else
										{
											si->use_download_directory = false;
										}
									}
								}

								char value[ 21 ];

								si->use_parts = ( _SendMessageW( g_hWnd_chk_sm_enable_download_parts, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

								if ( si->use_parts )
								{
									_SendMessageA( g_hWnd_sm_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
									si->parts = ( unsigned char )_strtoul( value, NULL, 10 );
								}

								si->use_download_speed_limit = ( _SendMessageW( g_hWnd_chk_sm_enable_speed_limit, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

								if ( si->use_download_speed_limit )
								{
									_SendMessageA( g_hWnd_sm_speed_limit, WM_GETTEXT, 21, ( LPARAM )value );
									si->download_speed_limit = strtoull( value );
								}

								si->ssl_version = ( char )_SendMessageW( g_hWnd_sm_ssl_version, CB_GETCURSEL, 0, 0 );

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_username, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length > 0 )
								{
									++edit_length; // Include the NULL terminator.
									si->w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * edit_length );
									_SendMessageW( g_hWnd_edit_sm_username, WM_GETTEXT, edit_length, ( LPARAM )si->w_username );
								}

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_password, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length > 0 )
								{
									++edit_length; // Include the NULL terminator.
									si->w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * edit_length );
									_SendMessageW( g_hWnd_edit_sm_password, WM_GETTEXT, edit_length, ( LPARAM )si->w_password );
								}

								unsigned int download_operation = ( unsigned int )_SendMessageW( g_hWnd_sm_download_operation, CB_GETCURSEL, 0, 0 );
								if ( download_operation == 1 )		{ download_operation = DOWNLOAD_OPERATION_ADD_STOPPED; }
								else if ( download_operation == 2 )	{ download_operation = DOWNLOAD_OPERATION_VERIFY; }
								else								{ download_operation = DOWNLOAD_OPERATION_NONE; }

								si->download_operations = ( download_operations | download_operation );

								//

								// COMMENTS, COOKIES, HEADERS, DATA

								wchar_t *edit;
								int utf8_length;

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_comments, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length > 0 )
								{
									si->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
									_SendMessageW( g_hWnd_edit_sm_comments, WM_GETTEXT, edit_length + 1, ( LPARAM )si->comments );
								}

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_cookies, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length > 0 )
								{
									edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
									_SendMessageW( g_hWnd_edit_sm_cookies, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

									utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
									si->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, edit, -1, si->utf8_cookies, utf8_length, NULL, NULL );

									GlobalFree( edit );
								}

								// Must be at least 2 characters long. "a:" is a valid header name and value.
								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_headers, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length >= 2 )
								{
									edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 + 2 ) );	// Add 2 for \r\n
									_SendMessageW( g_hWnd_edit_sm_headers, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

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
										si->utf8_headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
										WideCharToMultiByte( CP_UTF8, 0, edit_start, -1, si->utf8_headers, utf8_length, NULL, NULL );
									}

									GlobalFree( edit );
								}

								si->method = ( _SendMessageW( g_hWnd_chk_sm_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? METHOD_POST : 0 );

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_data, WM_GETTEXTLENGTH, 0, 0 );
								if ( edit_length > 0 )
								{
									edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
									_SendMessageW( g_hWnd_edit_sm_data, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

									utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
									si->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, edit, -1, si->utf8_data, utf8_length, NULL, NULL );

									GlobalFree( edit );
								}

								//

								// PROXY


								si->proxy_info.type = ( unsigned char )_SendMessageW( g_hWnd_sm_proxy_type, CB_GETCURSEL, 0, 0 );

								if ( si->proxy_info.type != 0 )
								{
									unsigned int hostname_length;

									si->proxy_info.address_type = ( _SendMessageW( g_hWnd_chk_sm_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

									if ( si->proxy_info.address_type == 0 )
									{
										hostname_length = ( unsigned int )_SendMessageW( g_hWnd_sm_hostname_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
										si->proxy_info.hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
										_SendMessageW( g_hWnd_sm_hostname_socks, WM_GETTEXT, hostname_length, ( LPARAM )si->proxy_info.hostname );

										if ( normaliz_state == NORMALIZ_STATE_RUNNING )
										{
											int punycode_length = _IdnToAscii( 0, si->proxy_info.hostname, hostname_length, NULL, 0 );

											if ( punycode_length > ( int )hostname_length )
											{
												si->proxy_info.punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
												_IdnToAscii( 0, si->proxy_info.hostname, hostname_length, si->proxy_info.punycode_hostname, punycode_length );
											}
										}
									}
									else
									{
										_SendMessageW( g_hWnd_sm_ip_address_socks, IPM_GETADDRESS, 0, ( LPARAM )&si->proxy_info.ip_address );
									}

									_SendMessageA( g_hWnd_sm_port_socks, WM_GETTEXT, 6, ( LPARAM )value );
									si->proxy_info.port = ( unsigned short )_strtoul( value, NULL, 10 );

									unsigned int auth_length;

									if ( si->proxy_info.type == 1 || si->proxy_info.type == 2 )	// HTTP and HTTPS
									{
										auth_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_GETTEXTLENGTH, 0, 0 );
										if ( auth_length > 0 )
										{
											si->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( auth_length + 1 ) );
											_SendMessageW( g_hWnd_edit_sm_proxy_auth_username, WM_GETTEXT, auth_length + 1, ( LPARAM )si->proxy_info.w_username );
										}

										auth_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_GETTEXTLENGTH, 0, 0 );
										if ( auth_length > 0 )
										{
											si->proxy_info.w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( auth_length + 1 ) );
											_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, WM_GETTEXT, auth_length + 1, ( LPARAM )si->proxy_info.w_password );
										}
									}
									else if ( si->proxy_info.type == 3 )	// SOCKS v4
									{
										si->proxy_info.resolve_domain_names = ( _SendMessageW( g_hWnd_chk_sm_resolve_domain_names_v4a, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

										auth_length = ( unsigned int )_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_GETTEXTLENGTH, 0, 0 );
										if ( auth_length > 0 )
										{
											si->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( auth_length + 1 ) );
											_SendMessageW( g_hWnd_sm_auth_ident_username_socks, WM_GETTEXT, auth_length + 1, ( LPARAM )si->proxy_info.w_username );
										}
									}
									else if ( si->proxy_info.type == 4 )	// SOCKS v5
									{
										si->proxy_info.resolve_domain_names = ( _SendMessageW( g_hWnd_chk_sm_resolve_domain_names, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

										si->proxy_info.use_authentication = ( _SendMessageW( g_hWnd_chk_sm_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

										if ( si->proxy_info.use_authentication )
										{
											auth_length = ( unsigned int )_SendMessageW( g_hWnd_sm_auth_username_socks, WM_GETTEXTLENGTH, 0, 0 );
											if ( auth_length > 0 )
											{
												si->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( auth_length + 1 ) );
												_SendMessageW( g_hWnd_sm_auth_username_socks, WM_GETTEXT, auth_length + 1, ( LPARAM )si->proxy_info.w_username );
											}

											auth_length = ( unsigned int )_SendMessageW( g_hWnd_sm_auth_password_socks, WM_GETTEXTLENGTH, 0, 0 );
											if ( auth_length > 0 )
											{
												si->proxy_info.w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( auth_length + 1 ) );
												_SendMessageW( g_hWnd_sm_auth_password_socks, WM_GETTEXT, auth_length + 1, ( LPARAM )si->proxy_info.w_password );
											}
										}
									}
								}

								//

								sui->si = si;

								// sui is freed in handle_site_list.
								HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_site_list, ( void * )sui, 0, NULL );
								if ( thread != NULL )
								{
									CloseHandle( thread );
								}
								else
								{
									FreeSiteInfo( &sui->si );
									GlobalFree( sui );
								}
							}
							else
							{
								GlobalFree( sui );
							}
						}
					}
					else
					{
						CMessageBoxW( hWnd, ST_V_The_specified_site_is_invalid, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
					}
				}
				break;

				case BTN_REMOVE_SITE:
				case MENU_SM_REMOVE_SEL:
				{
					if ( CMessageBoxW( hWnd, ST_V_PROMPT_remove_selected_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
					{
						SITE_UPDATE_INFO *sui = ( SITE_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SITE_UPDATE_INFO ) );
						if ( sui != NULL )
						{
							sui->update_type = 1;	// Remove
							sui->si = NULL;

							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_site_list, ( void * )sui, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								GlobalFree( sui );
							}
						}
					}
				}
				break;

				case BTN_CLOSE_SITE_MANAGER_WND:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				/*case EDIT_SITE:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_EnableWindow( g_hWnd_add_login, ( _SendMessageW( g_hWnd_edit_sm_site, WM_GETTEXTLENGTH, 0, 0 ) > 0 ? TRUE : FALSE ) );
					}
				}
				break;*/

				case MENU_SM_SELECT_ALL:
				{
					// Set the state of all items to selected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.state = LVIS_SELECTED;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_site_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );
				}
				break;

				case MENU_SM_ENABLE_SEL:
				case MENU_SM_DISABLE_SEL:
				{
					if ( ( int )_SendMessageW( g_hWnd_site_list, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
					{
						SITE_UPDATE_INFO *sui = ( SITE_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SITE_UPDATE_INFO ) );
						if ( sui != NULL )
						{
							if ( LOWORD( wParam ) == MENU_SM_ENABLE_SEL )
							{
								sui->enable = true;
							}
							else// if ( LOWORD( wParam ) == MENU_SM_DISABLE_SEL )
							{
								sui->enable = false;
							}

							sui->update_type = 2;	// Enable/Disable
							sui->si = NULL;

							// sui is freed in handle_site_list.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_site_list, ( void * )sui, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								GlobalFree( sui );
							}
						}
					}
				}
				break;

				case CHK_SHOW_PASSWORDS:
				{
					g_show_passwords = ( _SendMessageW( g_hWnd_chk_show_passwords, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					_SendMessageW( g_hWnd_edit_sm_proxy_auth_password, EM_SETPASSWORDCHAR, ( g_show_passwords ? 0 : g_default_pw_char ), 0 );
					_InvalidateRect( g_hWnd_edit_sm_proxy_auth_password, NULL, FALSE );

					_SendMessageW( g_hWnd_sm_auth_password_socks, EM_SETPASSWORDCHAR, ( g_show_passwords ? 0 : g_default_pw_char ), 0 );
					_InvalidateRect( g_hWnd_sm_auth_password_socks, NULL, FALSE );

					_SendMessageW( g_hWnd_edit_sm_password, EM_SETPASSWORDCHAR, ( g_show_passwords ? 0 : g_default_pw_char ), 0 );
					_InvalidateRect( g_hWnd_edit_sm_password, NULL, FALSE );

					_InvalidateRect( g_hWnd_site_list, NULL, FALSE );
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
				case LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Change the format of the items in the column if Ctrl is held while clicking the column.
					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						// Change the size column info.
						if ( nmlv->iSubItem == SM_COLUMN_DOWNLOAD_SPEED_LIMIT )
						{
							if ( sm_t_down_speed >= SIZE_FORMAT_AUTO )
							{
								sm_t_down_speed = SIZE_FORMAT_BYTE;
							}
							else
							{
								++sm_t_down_speed;
							}
							InvalidateRect( nmlv->hdr.hwndFrom, NULL, TRUE );
						}
					}
					else	// Normal column click. Sort the items in the column.
					{
						LVCOLUMN lvc;
						_memzero( &lvc, sizeof( LVCOLUMN ) );
						lvc.mask = LVCF_FMT | LVCF_ORDER;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						// The number column doesn't get sorted and its iOrder is always 0, so let's remove any sort arrows that are active.
						if ( lvc.iOrder == 0 )
						{
							// Remove the sort format for all columns.
							for ( unsigned char i = 1; _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc ) == TRUE; ++i )
							{
								// Remove sort up and sort down
								lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
								_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
							}

							break;
						}

						SORT_INFO si;
						si.column = lvc.iOrder;
						si.hWnd = nmlv->hdr.hwndFrom;

						if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
						{
							si.direction = 0;	// Now sort down.

							// Sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );
						}
						else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
						{
							si.direction = 1;	// Now sort up.

							// Sort up
							lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
						}
						else	// Column has no sorting set.
						{
							// Remove the sort format for all columns.
							for ( unsigned char i = 0; _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc ) == TRUE; ++i )
							{
								// Remove sort up and sort down
								lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
								_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
							}

							// Read current the format from the clicked column
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

							si.direction = 0;	// Start the sort going down.

							// Sort down to start.
							lvc.fmt = lvc.fmt | HDF_SORTDOWN;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
						}

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )LMCompareFunc );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_site_list )
					{
						POINT p;
						_GetCursorPos( &p );

						int item_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 );
						int sel_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 );

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.mask = LVIF_PARAM;
						lvi.iItem = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETNEXTITEM, ( WPARAM )-1, LVNI_FOCUSED | LVNI_SELECTED );
						_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEM, 0, ( LPARAM )&lvi );

						if ( lvi.iItem != -1 && lvi.lParam != NULL )
						{
							SITE_INFO *si = ( SITE_INFO * )lvi.lParam;

							_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_ENABLE_SEL, ( si->enable ? MF_GRAYED : MF_ENABLED ) );
							_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_DISABLE_SEL, ( si->enable ? MF_ENABLED : MF_GRAYED ) );
						}
						else
						{
							_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_ENABLE_SEL, MF_GRAYED );
							_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_DISABLE_SEL, MF_GRAYED );
						}

						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_REMOVE_SEL, ( sel_count > 0 ? MF_ENABLED : MF_GRAYED ) );
						_EnableMenuItem( g_hMenuSub_site_manager, MENU_SM_SELECT_ALL, ( sel_count == item_count ? MF_GRAYED : MF_ENABLED ) );

						_TrackPopupMenu( g_hMenuSub_site_manager, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( ( nmlv->uNewState != ( LVIS_FOCUSED | LVIS_SELECTED ) ) )
					//if ( !( nmlv->uNewState & LVIS_SELECTED ) )
					{
						//_EnableWindow( g_hWnd_remove_site, FALSE );

						//ResetTabPages();

						break;
					}

					/*if ( nmlv->lParam != NULL )
					{
						g_selected_site_info = ( SITE_INFO * )nmlv->lParam;
						g_selected_site_index = nmlv->iItem;
					}*/

					if ( nmlv->hdr.hwndFrom == g_hWnd_site_list )
					{
						/*int index;

						if ( _GetKeyState( VK_LBUTTON ) & 0x8000 )
						{
							// The nmlv->iItem value isn't correct if we're dragging down.
							// We need to do a hit test for the correct index.
							LVHITTESTINFO lvhti;
							_memzero( &lvhti, sizeof( LVHITTESTINFO  ) );
							_GetCursorPos( &lvhti.pt );
							_ScreenToClient( nmlv->hdr.hwndFrom, &lvhti.pt ); 
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SUBITEMHITTEST, 0, ( LPARAM )&lvhti );

							index = lvhti.iItem;
						}
						else
						{
							index = nmlv->iItem;
						}

						SelectSiteItem( index );*/

						SelectSiteItem( nmlv->iItem );
					}
				}
				break;

				case NM_CLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_site_list )
					{
						if ( nmitem->iItem == -1 )
						{
							if ( ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) == 0 )
							{
								g_selected_site_info = NULL;
								g_selected_site_index = -1;

								_EnableWindow( g_hWnd_remove_site, FALSE );

								ResetSMTabPages();
							}

							break;
						}

						//_EnableWindow( g_hWnd_save_site, enable );
						_EnableWindow( g_hWnd_remove_site, TRUE );

						//SITE_INFO *si = ( SITE_INFO * )nmlv->lParam;

						/*int index = ( int )_SendMessageW( g_hWnd_sm_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab

						BOOL enable = ( _SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 ? TRUE : FALSE );

						_EnableWindow( g_hWnd_update_login, enable );
						_EnableWindow( g_hWnd_remove_login, enable );*/

						SelectSiteItem( nmitem->iItem );
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 && !in_worker_thread )
					{
						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SM_SELECT_ALL, 0 );
								}
							}
							break;
						}
					}
					else if ( ( ( LPNMLVKEYDOWN )lParam )->wVKey == VK_DELETE )	// Remove items if Delete is down and there are selected items in the list.
					{
						if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_SM_REMOVE_SEL, 0 );
						}
					}
				}
				break;

				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					//NMHDR *nmhdr = ( NMHDR * )lParam;

					ShowHideSMTabs( SW_HIDE );
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

					ShowHideSMTabs( SW_SHOW );
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				SITE_INFO *si = ( SITE_INFO * )dis->itemData;

				// If an item is being deleted, then don't draw it.
				if ( skip_site_list_draw )
				{
					return TRUE;
				}

#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					// Alternate item color's background.
					HBRUSH color;
					if ( dis->itemID & 1 )	// Even rows will have a dark grey background.
					{
						color = _CreateSolidBrush( dm_color_edit_background );
					}
					else
					{
						color = _CreateSolidBrush( ( COLORREF )RGB( 0x00, 0x00, 0x00 ) );
					}
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}
				else
#endif
				{
					// Alternate item color's background.
					if ( dis->itemID & 1 )	// Even rows will have a light grey background.
					{
						HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
						_FillRect( dis->hDC, &dis->rcItem, color );
						_DeleteObject( color );
					}
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					HBRUSH color;

#ifdef ENABLE_DARK_MODE
					if ( g_use_dark_mode )
					{
						color = _CreateSolidBrush( dm_color_list_highlight );
					}
					else
#endif
					{
						color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					}

					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );

					selected = true;
				}

				wchar_t tbuf[ 128 ];

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int DT_ALIGN = 0;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i <= 17; ++i )
				{
					wchar_t *buf = GetSiteInfoString( si, i, dis->itemID + 1, tbuf, 128 );

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					if ( i == SM_COLUMN_DOWNLOAD_PARTS || i == SM_COLUMN_DOWNLOAD_SPEED_LIMIT )
					{
						DT_ALIGN = DT_RIGHT;
					}
					else
					{
						DT_ALIGN = DT_LEFT;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, i, ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					// This will adjust the text to fit nicely into the rectangle.
					last_rc.left = _SCALE_SM_( 5 ) + last_left;
					last_rc.right = lvc.cx + last_left - _SCALE_SM_( 5 );

					// Save the last left position of our column.
					last_left += lvc.cx;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					if ( width <= 0 )
					{
						continue;
					}

					int height = last_rc.bottom - last_rc.top;

					// Normal text position.
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = width;
					rc.bottom = height;

					// Create and save a bitmap in memory to paint to.
					HDC hdcMem = _CreateCompatibleDC( dis->hDC );
					HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
					HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
					_DeleteObject( ohbm );
					_DeleteObject( hbm );
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont_site_manager );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					HBRUSH color;

					// Draw selected text
					if ( selected )
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							color = _CreateSolidBrush( dm_color_list_highlight );
						}
						else
#endif
						{
							color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
						}

						// Fill the background.
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// White text.
						_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOW ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							// Fill the background.
							color = _CreateSolidBrush( ( dis->itemID & 1 ? dm_color_edit_background : ( COLORREF )RGB( 0x00, 0x00, 0x00 ) ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// White text.
							_SetTextColor( hdcMem, ( si->enable ? dm_color_window_text : ( COLORREF )RGB( 0xFF, 0x00, 0x00 ) ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
						}
						else
#endif
						{
							// Fill the background.
							color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// Black text.
							_SetTextColor( hdcMem, ( si->enable ? _GetSysColor( COLOR_WINDOWTEXT ) : ( COLORREF )RGB( 0xFF, 0x00, 0x00 ) ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
						}
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}

				if ( dis->itemState & ODS_FOCUS )
				{
					DWORD ui_state = ( DWORD )_SendMessageW( hWnd, WM_QUERYUISTATE, 0, 0 );
					if ( !( ui_state & UISF_HIDEFOCUS ) && dis->hwndItem == _GetFocus() )
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							LOGBRUSH lb;
							lb.lbColor = dm_color_focus_rectangle;
							lb.lbStyle = PS_SOLID;
							HPEN hPen = _ExtCreatePen( PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, NULL );
							HPEN old_color = ( HPEN )_SelectObject( dis->hDC, hPen );
							_DeleteObject( old_color );
							HBRUSH old_brush = ( HBRUSH )_SelectObject( dis->hDC, _GetStockObject( NULL_BRUSH ) );
							_DeleteObject( old_brush );
							_Rectangle( dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom - 1 );
							_DeleteObject( hPen );
						}
						else
#endif
						{
							RECT rc;
							rc.left = dis->rcItem.left;
							rc.top = dis->rcItem.top;
							rc.right = dis->rcItem.right;
							rc.bottom = dis->rcItem.bottom - 1;
							_DrawFocusRect( dis->hDC, &rc );
						}
					}
				}
			}

			return TRUE;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = _SCALE_SM_( MIN_WIDTH );
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = _SCALE_SM_( MIN_HEIGHT );
			
			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			if ( g_sm_use_theme && _IsThemeActive() == TRUE )
			{
				if ( ( HWND )lParam == g_hWnd_static_sm_category ||
					 ( HWND )lParam == g_hWnd_chk_sm_enable_download_directory ||
					 ( HWND )lParam == g_hWnd_chk_sm_enable_download_parts ||
					 ( HWND )lParam == g_hWnd_static_sm_ssl_version ||
					 ( HWND )lParam == g_hWnd_chk_sm_enable_speed_limit ||
					 ( HWND )lParam == g_hWnd_static_sm_username ||
					 ( HWND )lParam == g_hWnd_static_sm_password ||
					 ( HWND )lParam == g_hWnd_chk_sm_simulate_download ||
					 ( HWND )lParam == g_hWnd_static_sm_download_operation ||
					 ( HWND )lParam == g_hWnd_static_sm_comments ||
					 ( HWND )lParam == g_hWnd_static_sm_cookies ||
					 ( HWND )lParam == g_hWnd_static_sm_headers ||
					 ( HWND )lParam == g_hWnd_chk_sm_send_data ||
					 ( HWND )lParam == g_hWnd_static_sm_proxy_type ||
					 ( HWND )lParam == g_hWnd_static_sm_port_socks ||
					 ( HWND )lParam == g_hWnd_static_sm_colon_socks ||
					 ( HWND )lParam == g_hWnd_chk_sm_type_hostname_socks ||
					 ( HWND )lParam == g_hWnd_chk_sm_type_ip_address_socks ||
					 ( HWND )lParam == g_hWnd_static_sm_proxy_auth_username ||
					 ( HWND )lParam == g_hWnd_static_sm_proxy_auth_password ||
					 ( HWND )lParam == g_hWnd_static_sm_auth_ident_username_socks ||
					 ( HWND )lParam == g_hWnd_chk_sm_resolve_domain_names_v4a ||
					 ( HWND )lParam == g_hWnd_chk_sm_use_authentication_socks ||
					 ( HWND )lParam == g_hWnd_static_sm_auth_username_socks ||
					 ( HWND )lParam == g_hWnd_static_sm_auth_password_socks ||
					 ( HWND )lParam == g_hWnd_chk_sm_resolve_domain_names )
				{
					_SetBkMode( ( HDC )wParam, TRANSPARENT );

					if ( g_sm_draw_tab_pane )
					{
						POINT pt;
						pt.x = 0; pt.y = 0;

						_MapWindowPoints( g_hWnd_sm_tab, ( HWND )lParam, &pt, 1 );
						_SetBrushOrgEx( ( HDC )wParam, pt.x, pt.y, NULL );

						return ( INT_PTR )g_sm_tab_brush;
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

			int tab_width = rc.right - _SCALE_SM_( 20 );
			int tab_height = _SCALE_SM_( 247 );

			// This brush is refreshed whenever the tab changes size.
			// It's used to paint the background of static controls.
			// Windows XP has a gradient colored tab pane and setting the background of a static control to TRANSPARENT in WM_CTLCOLORSTATIC doesn't work well.
			if ( g_sm_draw_tab_pane && ( wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED ) && ( g_sm_tab_width != tab_width ) )
			{
				g_sm_tab_width = tab_width;

				HBRUSH old_brush = g_sm_tab_brush;

				HDC hDC = _GetDC( g_hWnd_sm_tab );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, g_sm_tab_width, tab_height );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );

				_SendMessageW( g_hWnd_sm_tab, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT ) );

				g_sm_tab_brush = _CreatePatternBrush( hbm );

				_DeleteObject( hbm );

				_DeleteDC( hdcMem );
				_ReleaseDC( g_hWnd_sm_tab, hDC );

				if ( old_brush != NULL )
				{
					_DeleteObject( old_brush );
				}
			}

			_SendMessageW( g_hWnd_sm_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			int tab_child_y_offset = ( rc_tab.bottom - rc_tab.top ) + ( rc.bottom - _SCALE_SM_( 338 ) );

			int spinner_width = _SCALE_SM_( site_manager_spinner_width );
			int spinner_height = _SCALE_SM_( site_manager_spinner_height );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 59 );

			_DeferWindowPos( hdwp, g_hWnd_site_list, HWND_BOTTOM, _SCALE_SM_( 10 ), _SCALE_SM_( 10 ), rc.right - _SCALE_SM_( 20 ), rc.bottom - _SCALE_SM_( 358 ), 0 );

			_DeferWindowPos( hdwp, g_hWnd_sm_tab, HWND_BOTTOM, _SCALE_SM_( 10 ), rc.bottom - _SCALE_SM_( 338 ), rc.right - _SCALE_SM_( 20 ), tab_height, 0 );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_sm_category, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 12 ), _SCALE_SM_( 120 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_category, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 30 ), _SCALE_SM_( 120 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_enable_download_directory, HWND_TOP, _SCALE_SM_( 164 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 400 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_download_directory, HWND_TOP, _SCALE_SM_( 164 ), tab_child_y_offset + _SCALE_SM_( 30 ), rc.right - _SCALE_SM_( 224 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_sm_download_directory, HWND_TOP, rc.right - _SCALE_SM_( 55 ), tab_child_y_offset + _SCALE_SM_( 30 ), _SCALE_SM_( 35 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_enable_download_parts, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 63 ), _SCALE_SM_( 120 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_download_parts, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 83 ), _SCALE_SM_( 100 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_sm_download_parts, HWND_TOP, _SCALE_SM_( 120 ), tab_child_y_offset + _SCALE_SM_( 83 ), spinner_width, spinner_height, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_ssl_version, HWND_TOP, _SCALE_SM_( 164 ), tab_child_y_offset + _SCALE_SM_( 65 ), _SCALE_SM_( 125 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_ssl_version, HWND_TOP, _SCALE_SM_( 164 ), tab_child_y_offset + _SCALE_SM_( 83 ), _SCALE_SM_( 100 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_enable_speed_limit, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 116 ), _SCALE_SM_( 250 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_speed_limit, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 136 ), _SCALE_SM_( 200 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_username, HWND_TOP, _SCALE_SM_( 301 ), tab_child_y_offset + _SCALE_SM_( 84 ), _SCALE_SM_( 120 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_username, HWND_TOP, _SCALE_SM_( 301 ), tab_child_y_offset + _SCALE_SM_( 102 ), _SCALE_SM_( 120 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_sm_password, HWND_TOP, _SCALE_SM_( 431 ), tab_child_y_offset + _SCALE_SM_( 84 ), _SCALE_SM_( 120 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_password, HWND_TOP, _SCALE_SM_( 431 ), tab_child_y_offset + _SCALE_SM_( 102 ), _SCALE_SM_( 120 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_simulate_download, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 169 ), _SCALE_SM_( 400 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_sm_download_operation, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 196 ), _SCALE_SM_( 180 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_download_operation, HWND_TOP, _SCALE_SM_( 205 ), tab_child_y_offset + _SCALE_SM_( 192 ), _SCALE_SM_( 100 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_sm_comments, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 400 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_comments, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 28 ), rc.right - _SCALE_SM_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_SM_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_cookies, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 400 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_cookies, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 28 ), rc.right - _SCALE_SM_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_SM_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_headers, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 400 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_headers, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 28 ), rc.right - _SCALE_SM_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_SM_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_send_data, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 400 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_data, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 30 ), rc.right - _SCALE_SM_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_SM_( 40 ), SWP_NOZORDER );


			//

			_DeferWindowPos( hdwp, g_hWnd_static_sm_proxy_type, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 10 ), _SCALE_SM_( 150 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_proxy_type, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 28 ), _SCALE_SM_( 100 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_hoz1, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 61 ), rc.right - _SCALE_SM_( 40 ), _SCALE_SM_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_type_hostname_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 69 ), _SCALE_SM_( 200 ), _SCALE_SM_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_sm_type_ip_address_socks, HWND_TOP, _SCALE_SM_( 225 ), tab_child_y_offset + _SCALE_SM_( 69 ), _SCALE_SM_( 110 ), _SCALE_SM_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_sm_hostname_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 89 ), _SCALE_SM_( 310 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_ip_address_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 89 ), _SCALE_SM_( 310 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_colon_socks, HWND_TOP, _SCALE_SM_( 331 ), tab_child_y_offset + _SCALE_SM_( 92 ), _SCALE_SM_( 8 ), _SCALE_SM_( 17 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_port_socks, HWND_TOP, _SCALE_SM_( 340 ), tab_child_y_offset + _SCALE_SM_( 71 ), _SCALE_SM_( 75 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_port_socks, HWND_TOP, _SCALE_SM_( 340 ), tab_child_y_offset + _SCALE_SM_( 89 ), _SCALE_SM_( 75 ), _SCALE_SM_( 23 ), SWP_NOZORDER );


			_DeferWindowPos( hdwp, g_hWnd_static_sm_proxy_auth_username, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 118 ), _SCALE_SM_( 150 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_proxy_auth_username, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 136 ), _SCALE_SM_( 150 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_sm_proxy_auth_password, HWND_TOP, _SCALE_SM_( 180 ), tab_child_y_offset + _SCALE_SM_( 118 ), _SCALE_SM_( 150 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_proxy_auth_password, HWND_TOP, _SCALE_SM_( 180 ), tab_child_y_offset + _SCALE_SM_( 136 ), _SCALE_SM_( 150 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			// v4

			_DeferWindowPos( hdwp, g_hWnd_static_sm_auth_ident_username_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 118 ), _SCALE_SM_( 400 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_auth_ident_username_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 136 ), _SCALE_SM_( 150 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_resolve_domain_names_v4a, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 164 ), rc.right - _SCALE_SM_( 40 ), _SCALE_SM_( 20 ), SWP_NOZORDER );


			// v5

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_use_authentication_socks, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 118 ), _SCALE_SM_( 400 ), _SCALE_SM_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_auth_username_socks, HWND_TOP, _SCALE_SM_( 35 ), tab_child_y_offset + _SCALE_SM_( 138 ), _SCALE_SM_( 150 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_auth_username_socks, HWND_TOP, _SCALE_SM_( 35 ), tab_child_y_offset + _SCALE_SM_( 156 ), _SCALE_SM_( 150 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sm_auth_password_socks, HWND_TOP, _SCALE_SM_( 195 ), tab_child_y_offset + _SCALE_SM_( 138 ), _SCALE_SM_( 150 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sm_auth_password_socks, HWND_TOP, _SCALE_SM_( 195 ), tab_child_y_offset + _SCALE_SM_( 156 ), _SCALE_SM_( 150 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_sm_resolve_domain_names, HWND_TOP, _SCALE_SM_( 20 ), tab_child_y_offset + _SCALE_SM_( 184 ), rc.right - _SCALE_SM_( 40 ), _SCALE_SM_( 20 ), SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_sm_site, HWND_TOP, _SCALE_SM_( 10 ), rc.bottom - _SCALE_SM_( 84 ), rc.right - _SCALE_SM_( 20 ), _SCALE_SM_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sm_site, HWND_TOP, _SCALE_SM_( 10 ), rc.bottom - _SCALE_SM_( 66 ), rc.right - _SCALE_SM_( 20 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_new_site, HWND_TOP, _SCALE_SM_( 10 ), rc.bottom - _SCALE_SM_( 33 ), _SCALE_SM_( 105 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_save_site, HWND_TOP, _SCALE_SM_( 120 ), rc.bottom - _SCALE_SM_( 33 ), _SCALE_SM_( 105 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_remove_site, HWND_TOP, _SCALE_SM_( 230 ), rc.bottom - _SCALE_SM_( 33 ), _SCALE_SM_( 105 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_show_passwords, HWND_TOP, _SCALE_SM_( 340 ), rc.bottom - _SCALE_SM_( 33 ), _SCALE_SM_( 130 ), _SCALE_SM_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_close_lm_wnd, HWND_TOP, rc.right - _SCALE_SM_( 90 ), rc.bottom - _SCALE_SM_( 33 ), _SCALE_SM_( 80 ), _SCALE_SM_( 23 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_site_manager;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_site_manager;
			current_dpi_site_manager = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_site_manager );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_site_manager );
			hFont_site_manager = hFont;

			// This stupid control doesn't adapt to the change in font size. It needs to be resized first.
			_SetWindowPos( g_hWnd_sm_ip_address_socks, HWND_TOP, 0, 0, _SCALE_SM_( 310 ), _SCALE_SM_( 23 ), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
			_DeleteObject( hFont_copy_sm_proxy );
			hFont_copy_sm_proxy = UpdateFont( current_dpi_site_manager );
			_SendMessageW( g_hWnd_sm_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_copy_sm_proxy, 0 );

			for ( int i = 0; i < 18; ++i )
			{
				int column_width = ( int )_SendMessageA( g_hWnd_site_list, LVM_GETCOLUMNWIDTH, ( WPARAM )i, 0 );
				column_width = MulDiv( column_width, current_dpi_site_manager, last_dpi );
				_SendMessageA( g_hWnd_site_list, LVM_SETCOLUMNWIDTH, ( WPARAM )i, MAKELPARAM( column_width, 0 ) );
			}

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_site_manager == 0 )
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

			last_dpi_site_manager = last_dpi;

			return 0;
		}
		break;

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType == ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = _SCALE_SM_( g_default_row_height );
			}
			return TRUE;
		}
		break;

		case WM_UPDATE_CATEGORY:
		{
			_SendMessageW( g_hWnd_sm_category, CB_SETCURSEL, 0, 0 );

			_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( CB_SM_CATEGORY/*_GetDlgCtrlID( g_hWnd_sm_category )*/, CBN_SELCHANGE ), 0 );

			return TRUE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				CMessageBoxW( hWnd, ST_V_The_specified_site_already_exists, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
			}
			else if ( wParam == 2 )
			{
				CMessageBoxW( hWnd, ST_V_A_protocol_must_be_supplied, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
			}
			else if ( wParam == 3 )
			{
				g_selected_site_info = NULL;
				g_selected_site_index = -1;

				_EnableWindow( g_hWnd_remove_site, FALSE );

				ResetSMTabPages();
			}
			else if ( wParam == 4 )
			{
				SITE_UPDATE_INFO *sui = ( SITE_UPDATE_INFO * )lParam;
				if ( sui->si != NULL )
				{
					g_selected_site_info = sui->si;
					g_selected_site_index = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETITEMCOUNT, 0, 0 ) - 1;

					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					lvi.state = 0;
					_SendMessageW( g_hWnd_site_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );	// Clear all states.

					lvi.state = LVIS_SELECTED;
					_SendMessageW( g_hWnd_site_list, LVM_SETITEMSTATE, g_selected_site_index, ( LPARAM )&lvi );

					_EnableWindow( g_hWnd_remove_site, TRUE );
				}
			}
			else// if ( wParam == 0 )
			{
				if ( _IsWindowVisible( hWnd ) == FALSE )
				{
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, load_window_category_list, ( void * )g_hWnd_sm_category, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
				}

				_ShowWindow( hWnd, SW_SHOWNORMAL );
				_SetForegroundWindow( hWnd );
			}

			_SetFocus( g_hWnd_edit_sm_site );

			return TRUE;
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

			_SetFocus( g_hWnd_site_list );

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
			g_selected_site_info = NULL;
			g_selected_site_index = -1;

			// Delete our font.
			_DeleteObject( hFont_site_manager );

			_DeleteObject( hFont_copy_sm_proxy );
			hFont_copy_sm_proxy = NULL;

			if ( g_sm_tab_brush != NULL )
			{
				_DeleteObject( g_sm_tab_brush );
				g_sm_tab_brush = NULL;
			}

			g_sm_tab_width = 0;
			g_sm_use_theme = true;

			if ( t_sm_download_directory != NULL )
			{
				GlobalFree( t_sm_download_directory );
				t_sm_download_directory = NULL;
			}

			g_show_passwords = false;

			_DestroyMenu( g_hMenuSub_site_manager );

			g_hWnd_site_list = NULL;
			g_hWnd_site_manager = NULL;

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
