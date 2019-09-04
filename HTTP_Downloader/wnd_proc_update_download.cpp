/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
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
#include "connection.h"
#include "list_operations.h"
#include "string_tables.h"

#include "wnd_proc.h"

#define EDIT_UPDATE_DOWNLOAD_PARTS	1000
#define CHK_UPDATE_SEND_DATA		1001
#define BTN_UPDATE_DOWNLOAD			1002
#define BTN_UPDATE_CANCEL			1003

DOWNLOAD_INFO *g_update_download_info = NULL;	// The current item that we want to update.

HWND g_hWnd_update_download = NULL;

HWND g_hWnd_edit_update_url = NULL;

HWND g_hWnd_static_update_download_parts = NULL;
HWND g_hWnd_update_download_parts = NULL;
HWND g_hWnd_ud_update_download_parts = NULL;

HWND g_hWnd_static_update_ssl_version = NULL;
HWND g_hWnd_update_ssl_version = NULL;

HWND g_hWnd_btn_update_authentication = NULL;
HWND g_hWnd_static_update_username = NULL;
HWND g_hWnd_edit_update_username = NULL;
HWND g_hWnd_static_update_password = NULL;
HWND g_hWnd_edit_update_password = NULL;

HWND g_hWnd_update_tab = NULL;

HWND g_hWnd_static_update_cookies = NULL;
HWND g_hWnd_edit_update_cookies = NULL;

HWND g_hWnd_static_update_headers = NULL;
HWND g_hWnd_edit_update_headers = NULL;

HWND g_hWnd_chk_update_send_data = NULL;
HWND g_hWnd_edit_update_data = NULL;

HWND g_hWnd_static_paused_download = NULL;

HWND g_hWnd_btn_update_download = NULL;
HWND g_hWnd_update_cancel = NULL;

WNDPROC UpdateProc = NULL;

unsigned char current_parts_num = 0;

LRESULT CALLBACK UpdateSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CHAR:
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
		break;

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

	return _CallWindowProcW( UpdateProc, hWnd, msg, wParam, lParam );
}

void ShowHideUpdateTabs( int sw_type )
{
	int index = ( int )_SendMessageW( g_hWnd_update_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
	if ( index != -1 )
	{
		switch ( index )
		{
			case 0:
			{
				_ShowWindow( g_hWnd_static_update_cookies, sw_type );
				_ShowWindow( g_hWnd_edit_update_cookies, sw_type );
			}
			break;

			case 1:
			{
				_ShowWindow( g_hWnd_static_update_headers, sw_type );
				_ShowWindow( g_hWnd_edit_update_headers, sw_type );
			}
			break;

			case 2:
			{
				_ShowWindow( g_hWnd_chk_update_send_data, sw_type );
				_ShowWindow( g_hWnd_edit_update_data, sw_type );
			}
			break;
		}
	}
}

LRESULT CALLBACK UpdateDownloadWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			HWND hWnd_static_update_url = _CreateWindowW( WC_STATIC, ST_V_URL_, WS_CHILD | WS_VISIBLE, 20, 20, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_update_url = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			// Owner draw the static control. It causes the entire window to flicker when it's disabled.
			g_hWnd_static_update_download_parts = _CreateWindowW( WC_STATIC, ST_V_Download_parts_, SS_OWNERDRAW | WS_CHILD | WS_VISIBLE, 20, 68, 115, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_update_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 20, 83, 85, 23, hWnd, ( HMENU )EDIT_UPDATE_DOWNLOAD_PARTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_update_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 105, 82, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_update_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_update_download_parts, 0 );
            _SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETPOS, 0, 1 );

			g_hWnd_static_update_ssl_version = _CreateWindowW( WC_STATIC, ST_V_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 140, 68, 115, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_update_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 140, 83, 100, 23, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_update_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_update_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_update_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_update_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_update_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_update_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			g_hWnd_btn_update_authentication = _CreateWindowW( WC_BUTTON, ST_V_Authentication, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 260, 68, 230, 65, hWnd, NULL, NULL, NULL );

			g_hWnd_static_update_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 270, 83, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_update_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 270, 98, 100, 23, hWnd, NULL, NULL, NULL );

			g_hWnd_static_update_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 380, 83, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_update_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 380, 98, 100, 23, hWnd, NULL, NULL, NULL );

			g_hWnd_update_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | /*WS_CLIPCHILDREN |*/ WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_V_Cookies;
			_SendMessageW( g_hWnd_update_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Headers;
			_SendMessageW( g_hWnd_update_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_POST_Data;
			_SendMessageW( g_hWnd_update_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			g_hWnd_static_update_cookies = _CreateWindowW( WC_STATIC, ST_V_Cookies_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_update_cookies = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_update_headers = _CreateWindowW( WC_STATIC, ST_V_Headers_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_update_headers = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_update_send_data = _CreateWindowW( WC_BUTTON, ST_V_Send_POST_Data_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_UPDATE_SEND_DATA, NULL, NULL );
			g_hWnd_edit_update_data = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_paused_download = _CreateWindowW( WC_STATIC, ST_V_The_download_will_be_resumed, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_btn_update_download = _CreateWindowW( WC_BUTTON, ST_V_Update, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_UPDATE_DOWNLOAD, NULL, NULL );
			g_hWnd_update_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_UPDATE_CANCEL, NULL, NULL );

			//_SetFocus( g_hWnd_edit_update_url );

			_SendMessageW( g_hWnd_btn_update_download, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_update_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_paused_download, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_update_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_update_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_update_url, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_url, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_update_authentication, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_update_tab, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_cookies, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_cookies, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_update_headers, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_headers, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_update_send_data, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_update_data, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			UpdateProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_edit_update_cookies, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_edit_update_cookies, GWLP_WNDPROC, ( LONG_PTR )UpdateSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_update_headers, GWLP_WNDPROC, ( LONG_PTR )UpdateSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_update_data, GWLP_WNDPROC, ( LONG_PTR )UpdateSubProc );

			TabProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_update_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_update_tab, GWLP_WNDPROC, ( LONG_PTR )TabSubProc );

			// Open theme data. Must be done after we subclass the control.
			// Theme object will be closed when all tab controls are destroyed.
			_SendMessageW( g_hWnd_update_tab, WM_PROPAGATE, 0, ( LPARAM )COLOR_WINDOW );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			if ( ( HWND )lParam != g_hWnd_static_paused_download )
			{
				return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
			}
			else
			{
				return _DefWindowProcW( hWnd, msg, wParam, lParam );
			}
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The disabled static control causes flickering. The only way around it is to draw it ourselves.
			if ( dis->CtlType == ODT_STATIC )
			{
				_FillRect( dis->hDC, &dis->rcItem, _GetSysColorBrush( COLOR_WINDOW ) );

				COLORREF font_color = _GetSysColor( COLOR_WINDOWTEXT );
				if ( _IsWindowEnabled( dis->hwndItem ) == FALSE )
				{
					_SetTextColor( dis->hDC, _GetSysColor( COLOR_GRAYTEXT ) );
				}
				_DrawTextW( dis->hDC, ST_V_Download_parts_, ST_L_Download_parts_, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
			}

			return TRUE;
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

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( 11 );

			_DeferWindowPos( hdwp, g_hWnd_edit_update_url, HWND_TOP, 20, 35, rc.right - 40, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_update_tab, HWND_TOP, 20, 133, rc.right - 40, rc.bottom - 183, SWP_NOZORDER );

			//

			RECT rc_tab;
			_SendMessageW( g_hWnd_update_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			_DeferWindowPos( hdwp, g_hWnd_static_update_cookies, HWND_TOP, 30, 143 + rc_tab.bottom, rc.right - 60, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_update_cookies, HWND_TOP, 30, 158 + rc_tab.bottom, rc.right - 60, rc.bottom - 238, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_update_headers, HWND_TOP, 30, 143 + rc_tab.bottom, rc.right - 60, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_update_headers, HWND_TOP, 30, 158 + rc_tab.bottom, rc.right - 60, rc.bottom - 238, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_update_send_data, HWND_TOP, 30, 143 + rc_tab.bottom, rc.right - 60, 20, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_update_data, HWND_TOP, 30, 163 + rc_tab.bottom, rc.right - 60, rc.bottom - 243, SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_paused_download, HWND_TOP, 10, rc.bottom - 27, rc.right - 195, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_update_download, HWND_TOP, rc.right - 175, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_update_cancel, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = 525;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = 403;

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_UPDATE_DOWNLOAD:
				{
					int utf8_length = 0;
					wchar_t *edit = NULL;

					ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );

					// URL
					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_url, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_update_url, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						if ( _memcmp( g_update_download_info->url, edit, sizeof( wchar_t ) * ( edit_length + 1 ) ) == 0 )
						{
							GlobalFree( edit );
							ai->urls = NULL;
						}
						else
						{
							ai->urls = edit;
						}
					}
					else
					{
						ai->urls = NULL;
					}

					ai->ssl_version = ( char )_SendMessageW( g_hWnd_update_ssl_version, CB_GETCURSEL, 0, 0 );

					ai->method = ( _SendMessageW( g_hWnd_chk_update_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? METHOD_POST : METHOD_GET );

					char value[ 11 ];
					_SendMessageA( g_hWnd_update_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned char parts_limit = ( unsigned char )_strtoul( value, NULL, 10 );
					if ( parts_limit != current_parts_num )
					{
						ai->parts = parts_limit;
					}

					// Username
					edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_username, WM_GETTEXTLENGTH, 0, 0 );
					if ( edit_length > 0 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_update_username, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
						ai->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
						WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.username, utf8_length, NULL, NULL );

						GlobalFree( edit );
					}
					else
					{
						ai->auth_info.username = NULL;
					}

					// Password
					edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_password, WM_GETTEXTLENGTH, 0, 0 );
					if ( edit_length > 0 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_update_password, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
						ai->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
						WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.password, utf8_length, NULL, NULL );

						GlobalFree( edit );
					}
					else
					{
						ai->auth_info.password = NULL;
					}

					edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_cookies, WM_GETTEXTLENGTH, 0, 0 );
					if ( edit_length > 0 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_update_cookies, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
						ai->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
						WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_cookies, utf8_length, NULL, NULL );

						GlobalFree( edit );
					}
					else
					{
						ai->utf8_cookies = NULL;
					}

					// Must be at least 2 characters long. "a:" is a valid header name and value.
					edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_headers, WM_GETTEXTLENGTH, 0, 0 );
					if ( edit_length >= 2 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 + 2 ) );	// Add 2 for \r\n
						_SendMessageW( g_hWnd_edit_update_headers, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

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
					else
					{
						ai->utf8_headers = NULL;
					}

					edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_update_data, WM_GETTEXTLENGTH, 0, 0 );
					if ( edit_length > 0 )
					{
						edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
						_SendMessageW( g_hWnd_edit_update_data, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

						utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
						ai->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
						WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_data, utf8_length, NULL, NULL );

						GlobalFree( edit );
					}
					else
					{
						ai->utf8_data = NULL;
					}

					// ai is freed in handle_download_update.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_update, ( void * )ai, 0, NULL );	// Update selected download (stops and resumes the download).
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						// This is all that should be set for handle_download_update.
						GlobalFree( ai->utf8_data );
						GlobalFree( ai->utf8_headers );
						GlobalFree( ai->utf8_cookies );
						GlobalFree( ai->auth_info.username );
						GlobalFree( ai->auth_info.password );
						GlobalFree( ai->urls );
						GlobalFree( ai );

						// We couldn't update this.
						g_update_download_info = NULL;
					}

					// Don't WM_CLOSE since we don't want to set g_update_download_info to NULL. 
					_DestroyWindow( hWnd );
				}
				break;

				case BTN_UPDATE_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_UPDATE_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > current_parts_num )
						{
							__snprintf( value, 11, "%lu", current_parts_num );

							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

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

				case CHK_UPDATE_SEND_DATA:
				{
					_EnableWindow( g_hWnd_edit_update_data, ( _SendMessageW( g_hWnd_chk_update_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
				}
				break;
			}

			return 0;
		}

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					//NMHDR *nmhdr = ( NMHDR * )lParam;

					ShowHideUpdateTabs( SW_HIDE );
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

					ShowHideUpdateTabs( SW_SHOW );
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( lParam != NULL )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lParam;

				g_update_download_info = di;

				current_parts_num = di->parts;

				_SendMessageW( g_hWnd_edit_update_url, WM_SETTEXT, 0, ( LPARAM )di->url );

				_SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETRANGE32, 1, di->parts );
				_SendMessageW( g_hWnd_ud_update_download_parts, UDM_SETPOS, 0, ( di->parts_limit != 0 ? di->parts_limit : di->parts ) );

				BOOL enable = ( di->parts == 1 ? FALSE : TRUE );
				_EnableWindow( g_hWnd_static_update_download_parts, enable );
				_EnableWindow( g_hWnd_update_download_parts, enable );
				_EnableWindow( g_hWnd_ud_update_download_parts, enable ); // This actually disables itself if the range is 1.

				_SendMessageW( g_hWnd_update_ssl_version, CB_SETCURSEL, di->ssl_version, 0 );

				_SendMessageA( g_hWnd_edit_update_username, WM_SETTEXT, 0, ( LPARAM )di->auth_info.username );
				_SendMessageA( g_hWnd_edit_update_password, WM_SETTEXT, 0, ( LPARAM )di->auth_info.password );

				_SendMessageA( g_hWnd_edit_update_cookies, WM_SETTEXT, 0, ( LPARAM )di->cookies );
				_SendMessageA( g_hWnd_edit_update_headers, WM_SETTEXT, 0, ( LPARAM )di->headers );
				_SendMessageA( g_hWnd_edit_update_data, WM_SETTEXT, 0, ( LPARAM )di->data );

				_SendMessageW( g_hWnd_chk_update_send_data, BM_SETCHECK, ( di->method == METHOD_POST ? BST_CHECKED : BST_UNCHECKED ), 0 );
				_EnableWindow( g_hWnd_edit_update_data, ( di->method == METHOD_POST ? TRUE : FALSE ) );

				_ShowWindow( g_hWnd_static_paused_download, ( IS_STATUS( di->status, STATUS_PAUSED ) ? SW_SHOW : SW_HIDE ) );
			}

			//_EnableWindow( g_hWnd_main, FALSE );

			_ShowWindow( hWnd, SW_SHOWNORMAL );

			_SetForegroundWindow( hWnd );
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
			g_update_download_info = NULL;

			//_EnableWindow( g_hWnd_main, TRUE );

			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY_ALT:
		{
			_DestroyWindow( hWnd );
		}
		break;

		case WM_DESTROY:
		{
			current_parts_num = 0;

			g_hWnd_update_download = NULL;

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
