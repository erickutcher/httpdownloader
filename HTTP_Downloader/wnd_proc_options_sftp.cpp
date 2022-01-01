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
#include "utilities.h"
#include "lite_comctl32.h"
#include "lite_gdi32.h"

#define KEEP_ALIVE_MAX				172800
#define KEEP_ALIVE_MAX_S			"172800"

#define REKEY_MAX					2880
#define REKEY_MAX_S					"2880"

#define GSS_REKEY_MAX				2880
#define GSS_REKEY_MAX_S				"2880"

#define BTN_ENABLE_COMPRESSION				1000
#define BTN_ATTEMPT_GSSAPI_AUTHENTICATION	1001
#define BTN_ATTEMPT_GSSAPI_KEY_EXCHANGE		1002

#define EDIT_SFTP_KEEP_ALIVE_TIME			1003
#define EDIT_SFTP_REKEY_TIME				1004
#define EDIT_SFTP_GSS_REKEY_TIME			1005
#define EDIT_SFTP_REKEY_DATA_LIMIT			1006

// SFTP Tab
HWND g_hWnd_sftp_kex_algorithm = NULL;
HWND g_hWnd_sftp_host_key = NULL;
HWND g_hWnd_sftp_encryption_cipher = NULL;

HWND g_hWnd_chk_enable_compression = NULL;
HWND g_hWnd_chk_attempt_gssapi_authentication = NULL;
HWND g_hWnd_chk_attempt_gssapi_key_exchange = NULL;

HWND g_hWnd_sftp_keep_alive_time = NULL;
HWND g_hWnd_ud_sftp_keep_alive_time = NULL;

HWND g_hWnd_sftp_rekey_time = NULL;
HWND g_hWnd_ud_sftp_rekey_time = NULL;

HWND g_hWnd_sftp_gss_rekey_time = NULL;
HWND g_hWnd_ud_sftp_gss_rekey_time = NULL;

HWND g_hWnd_sftp_rekey_data_limit = NULL;

wchar_t rekey_data_limit_tooltip_text[ 32 ];
HWND g_hWnd_rekey_data_limit_tooltip = NULL;

HWND g_hWnd_tv_current = NULL;
HTREEITEM g_tv_start_item = NULL;
HTREEITEM g_tv_end_item = NULL;
bool g_tv_dragging = false;
bool g_tv_oob = false;	// Out of bounds.
RECT g_lb_item_rc;

WNDPROC PriorityTreeViewProc = NULL;

unsigned char g_priority_kex_algorithm[ KEX_ALGORITHM_COUNT ];
unsigned char g_priority_host_key[ HOST_KEY_COUNT ];
unsigned char g_priority_encryption_cipher[ ENCRYPTION_CIPHER_COUNT ];

void SetPriorityList( HWND hWnd, unsigned char *priority_list )
{
	if ( priority_list != NULL )
	{
		HTREEITEM hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_GETNEXTITEM, TVGN_ROOT, NULL );
		if ( hti != NULL )
		{
			char i = 0;

			TVITEM tvi;
			_memzero( &tvi, sizeof( TVITEM ) );

			do
			{
				tvi.mask = TVIF_PARAM | TVIF_STATE;
				tvi.stateMask = TVIS_STATEIMAGEMASK;
				tvi.hItem = hti;
				_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

				// Bits 1 to 6 are used for priority values.
				// Bits 7 and 8 are used for states. Right now it's either checked or unchecked. Maybe we'll have a third state later...
				priority_list[ i ] = ( ( unsigned char )tvi.lParam & 0x3F ) | ( ( ( tvi.state >> 12 ) - 1 ) ? 0x40 : 0x00 );

				++i;

				hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_GETNEXTITEM, TVGN_NEXT, ( LPARAM )hti );
			}
			while ( hti != NULL );
		}
	}
}

LRESULT CALLBACK PriorityTreeViewSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_LBUTTONDOWN:
		{
			TVHITTESTINFO tvht;
			_memzero( &tvht, sizeof( TVHITTESTINFO ) );
			tvht.pt.x = GET_X_LPARAM( lParam );
			tvht.pt.y = GET_Y_LPARAM( lParam );
			HTREEITEM hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_HITTEST, 0, ( LPARAM )&tvht );
			if ( hti != NULL )
			{
				if ( tvht.flags & TVHT_ONITEMSTATEICON )
				{
					TVITEM tvi;
					_memzero( &tvi, sizeof( TVITEM ) );
					tvi.mask = TVIF_PARAM;
					tvi.hItem = hti;
					_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

					unsigned char *priority;
					char count;
					if ( hWnd == g_hWnd_sftp_kex_algorithm ) { priority = g_priority_kex_algorithm; count = KEX_ALGORITHM_COUNT; }
					else if ( hWnd == g_hWnd_sftp_host_key ) { priority = g_priority_host_key; count = HOST_KEY_COUNT; }
					else if ( hWnd == g_hWnd_sftp_encryption_cipher ) { priority = g_priority_encryption_cipher; count = ENCRYPTION_CIPHER_COUNT; }
					else { priority = NULL; count = 0; }
					for ( char i = 0; i < count; ++i )
					{
						if ( ( priority[ i ] & 0x3F ) == ( ( unsigned char )tvi.lParam & 0x3F ) )
						{
							priority[ i ] ^= 0x40;	// Toggle checked/unchecked bit.
							break;
						}
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}

				_SendMessageW( hWnd, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti );
				_SetFocus( hWnd );
			}
		}
		break;
	}

	return _CallWindowProcW( PriorityTreeViewProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK SFTPTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_algorithm_policies = _CreateWindowW( WC_BUTTON, ST_V_Algorithm_Selection_Policies, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, rc.right, 200, hWnd, NULL, NULL, NULL );

			HWND hWnd_static_sftp_kex_algorithm = _CreateWindowW( WC_STATIC, ST_V_Key_Group_exchange_, WS_CHILD | WS_VISIBLE, 10, 20, 205, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_kex_algorithm = _CreateWindowExW( WS_EX_CLIENTEDGE | TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_SHOWSELALWAYS | TVS_FULLROWSELECT | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, 35, 205, 135, hWnd, NULL, NULL, NULL );
			HWND hWnd_static_sftp_host_key = _CreateWindowW( WC_STATIC, ST_V_Host_key_, WS_CHILD | WS_VISIBLE, 225, 20, 160, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_host_key = _CreateWindowExW( WS_EX_CLIENTEDGE | TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_SHOWSELALWAYS | TVS_FULLROWSELECT | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 225, 35, 160, 135, hWnd, NULL, NULL, NULL );
			HWND hWnd_static_sftp_encryption_cipher = _CreateWindowW( WC_STATIC, ST_V_Encryption_cipher_, WS_CHILD | WS_VISIBLE, 395, 20, 160, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_encryption_cipher = _CreateWindowExW( WS_EX_CLIENTEDGE | TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_SHOWSELALWAYS | TVS_FULLROWSELECT | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 395, 35, 160, 135, hWnd, NULL, NULL, NULL );

			// Checboxes don't get set when the style is applied in CreateWindow().
			// https://docs.microsoft.com/en-us/windows/win32/controls/tree-view-control-window-styles
			DWORD style = ( DWORD )_GetWindowLongPtrW( g_hWnd_sftp_kex_algorithm, GWL_STYLE );
			_SetWindowLongPtrW( g_hWnd_sftp_kex_algorithm, GWL_STYLE, style | TVS_CHECKBOXES );
			style = ( DWORD )_GetWindowLongPtrW( g_hWnd_sftp_host_key, GWL_STYLE );
			_SetWindowLongPtrW( g_hWnd_sftp_host_key, GWL_STYLE, style | TVS_CHECKBOXES );
			style = ( DWORD )_GetWindowLongPtrW( g_hWnd_sftp_encryption_cipher, GWL_STYLE );
			_SetWindowLongPtrW( g_hWnd_sftp_encryption_cipher, GWL_STYLE, style | TVS_CHECKBOXES );

			TVINSERTSTRUCT tvis;
			_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

			tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			tvis.item.stateMask = TVIS_EXPANDED | TVIS_STATEIMAGEMASK;
			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_FIRST;
			for ( char i = 0; i < KEX_ALGORITHM_COUNT; ++i )
			{
				g_priority_kex_algorithm[ i ] = cfg_priority_kex_algorithm[ i ];

				tvis.item.state = TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK( ( ( cfg_priority_kex_algorithm[ i ] & 0x40 ) ? 2 : 1 ) );
				tvis.item.pszText = sftp_kex_string_table[ ( cfg_priority_kex_algorithm[ i ] & 0x3F ) - 1 ].value;
				tvis.item.lParam = ( LPARAM )cfg_priority_kex_algorithm[ i ];
				HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_sftp_kex_algorithm, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
				tvis.hInsertAfter = hti;
			}

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_FIRST;
			for ( char i = 0; i < HOST_KEY_COUNT; ++i )
			{
				g_priority_host_key[ i ] = cfg_priority_host_key[ i ];

				tvis.item.state = TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK( ( ( cfg_priority_host_key[ i ] & 0x40 ) ? 2 : 1 ) );
				tvis.item.pszText = sftp_hk_string_table[ ( cfg_priority_host_key[ i ] & 0x3F ) - 1 ].value;
				tvis.item.lParam = ( LPARAM )cfg_priority_host_key[ i ];
				HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_sftp_host_key, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
				tvis.hInsertAfter = hti;
			}

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_FIRST;
			for ( char i = 0; i < ENCRYPTION_CIPHER_COUNT; ++i )
			{
				g_priority_encryption_cipher[ i ] = cfg_priority_encryption_cipher[ i ];

				tvis.item.state = TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK( ( ( cfg_priority_encryption_cipher[ i ] & 0x40 ) ? 2 : 1 ) );
				tvis.item.pszText = sftp_ec_string_table[ ( cfg_priority_encryption_cipher[ i ] & 0x3F ) - 1 ].value;
				tvis.item.lParam = ( LPARAM )cfg_priority_encryption_cipher[ i ];
				HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_sftp_encryption_cipher, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
				tvis.hInsertAfter = hti;
			}

			HWND hWnd_static_drag_order = _CreateWindowW( WC_STATIC, ST_V_Drag_items_to_reorder_priority, WS_CHILD | WS_VISIBLE, 10, 175, rc.right - 20, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_enable_compression = _CreateWindowW( WC_BUTTON, ST_V_Enable_compression, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 210, rc.right, 20, hWnd, ( HMENU )BTN_ENABLE_COMPRESSION, NULL, NULL );
			g_hWnd_chk_attempt_gssapi_authentication = _CreateWindowW( WC_BUTTON, ST_V_Attempt_GSSAPI_authentication, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 230, rc.right, 20, hWnd, ( HMENU )BTN_ATTEMPT_GSSAPI_AUTHENTICATION, NULL, NULL );
			g_hWnd_chk_attempt_gssapi_key_exchange = _CreateWindowW( WC_BUTTON, ST_V_Attempt_GSSAPI_key_exchange, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 250, rc.right, 20, hWnd, ( HMENU )BTN_ATTEMPT_GSSAPI_KEY_EXCHANGE, NULL, NULL );

			//

			HWND hWnd_sftp_send_keep_alive = _CreateWindowW( WC_STATIC, ST_V_Send_keep_alive_requests__seconds__, WS_CHILD | WS_VISIBLE, 0, 279, 350, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_keep_alive_time = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 275, 100, 23, hWnd, ( HMENU )EDIT_SFTP_KEEP_ALIVE_TIME, NULL, NULL );
			g_hWnd_ud_sftp_keep_alive_time = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_sftp_keep_alive_time, EM_LIMITTEXT, 6, 0 );
			_SendMessageW( g_hWnd_ud_sftp_keep_alive_time, UDM_SETBUDDY, ( WPARAM )g_hWnd_sftp_keep_alive_time, 0 );
			_SendMessageW( g_hWnd_ud_sftp_keep_alive_time, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_sftp_keep_alive_time, UDM_SETRANGE32, 0, KEEP_ALIVE_MAX );	// 0 = disabled, 1 second to 2 days max.
			_SendMessageW( g_hWnd_ud_sftp_keep_alive_time, UDM_SETPOS, 0, cfg_sftp_keep_alive_time );

			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_sftp_keep_alive_time, &rc_spinner );
			int spinner_width = rc_spinner.right - rc_spinner.left;

			_SetWindowPos( g_hWnd_sftp_keep_alive_time, HWND_TOP, rc.right - ( 100 + spinner_width ), 275, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_sftp_keep_alive_time, HWND_TOP, rc.right - spinner_width, 275, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//

			HWND hWnd_sftp_rekey = _CreateWindowW( WC_STATIC, ST_V_Rekey_time__minutes__, WS_CHILD | WS_VISIBLE, 0, 307, 350, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_rekey_time = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 303, 100, 23, hWnd, ( HMENU )EDIT_SFTP_REKEY_TIME, NULL, NULL );
			g_hWnd_ud_sftp_rekey_time = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_sftp_rekey_time, EM_LIMITTEXT, 6, 0 );
			_SendMessageW( g_hWnd_ud_sftp_rekey_time, UDM_SETBUDDY, ( WPARAM )g_hWnd_sftp_rekey_time, 0 );
			_SendMessageW( g_hWnd_ud_sftp_rekey_time, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_sftp_rekey_time, UDM_SETRANGE32, 0, REKEY_MAX );	// 0 = no limit, 1 minute to 2 days max.
			_SendMessageW( g_hWnd_ud_sftp_rekey_time, UDM_SETPOS, 0, cfg_sftp_rekey_time );

			_SetWindowPos( g_hWnd_sftp_rekey_time, HWND_TOP, rc.right - ( 100 + spinner_width ), 303, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_sftp_rekey_time, HWND_TOP, rc.right - spinner_width, 303, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//

			HWND hWnd_sftp_gss_rekey = _CreateWindowW( WC_STATIC, ST_V_GSS_rekey_time__minutes__, WS_CHILD | WS_VISIBLE, 0, 335, 350, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_gss_rekey_time = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 331, 100, 23, hWnd, ( HMENU )EDIT_SFTP_GSS_REKEY_TIME, NULL, NULL );
			g_hWnd_ud_sftp_gss_rekey_time = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_sftp_gss_rekey_time, EM_LIMITTEXT, 6, 0 );
			_SendMessageW( g_hWnd_ud_sftp_gss_rekey_time, UDM_SETBUDDY, ( WPARAM )g_hWnd_sftp_gss_rekey_time, 0 );
			_SendMessageW( g_hWnd_ud_sftp_gss_rekey_time, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_sftp_gss_rekey_time, UDM_SETRANGE32, 0, GSS_REKEY_MAX );	// 0 = no limit, 1 minute to 2 days max.
			_SendMessageW( g_hWnd_ud_sftp_gss_rekey_time, UDM_SETPOS, 0, cfg_sftp_gss_rekey_time );

			_SetWindowPos( g_hWnd_sftp_gss_rekey_time, HWND_TOP, rc.right - ( 100 + spinner_width ), 331, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_sftp_gss_rekey_time, HWND_TOP, rc.right - spinner_width, 331, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//

			HWND hWnd_static_sftp_rekey_data_limit = _CreateWindowW( WC_STATIC, ST_V_Rekey_data_limit__bytes__, WS_CHILD | WS_VISIBLE, 0, 364, 350, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_sftp_rekey_data_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 359, 100, 23, hWnd, ( HMENU )EDIT_SFTP_REKEY_DATA_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_sftp_rekey_data_limit, EM_LIMITTEXT, 10, 0 );

			g_hWnd_rekey_data_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			rekey_data_limit_tooltip_text[ 0 ] = 0;

			TOOLINFO ti;
			_memzero( &ti, sizeof( TOOLINFO ) );
			ti.cbSize = sizeof( TOOLINFO );
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = g_hWnd_sftp_rekey_data_limit;
			ti.lpszText = rekey_data_limit_tooltip_text;

			_GetClientRect( hWnd, &ti.rect );
			_SendMessageW( g_hWnd_rekey_data_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&ti );

			char value[ 11 ];
			_memzero( value, sizeof( char ) * 11 );
			__snprintf( value, 11, "%lu", cfg_sftp_rekey_data_limit );
			_SendMessageA( g_hWnd_sftp_rekey_data_limit, WM_SETTEXT, 0, ( LPARAM )value );

			//

			PriorityTreeViewProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_sftp_kex_algorithm, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_sftp_kex_algorithm, GWLP_WNDPROC, ( LONG_PTR )PriorityTreeViewSubProc );
			_SetWindowLongPtrW( g_hWnd_sftp_host_key, GWLP_WNDPROC, ( LONG_PTR )PriorityTreeViewSubProc );
			_SetWindowLongPtrW( g_hWnd_sftp_encryption_cipher, GWLP_WNDPROC, ( LONG_PTR )PriorityTreeViewSubProc );

			//

			_SendMessageW( hWnd_static_algorithm_policies, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_sftp_kex_algorithm, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_kex_algorithm, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_sftp_host_key, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_host_key, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_sftp_encryption_cipher, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_encryption_cipher, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_drag_order, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_enable_compression, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_attempt_gssapi_authentication, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_attempt_gssapi_key_exchange, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_sftp_send_keep_alive, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_keep_alive_time, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_sftp_rekey, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_rekey_time, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_sftp_gss_rekey, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_gss_rekey_time, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_sftp_rekey_data_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sftp_rekey_data_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			// Set settings.

			_SendMessageW( g_hWnd_chk_enable_compression, BM_SETCHECK, ( cfg_sftp_enable_compression ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_attempt_gssapi_authentication, BM_SETCHECK, ( cfg_sftp_attempt_gssapi_authentication ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_attempt_gssapi_key_exchange, BM_SETCHECK, ( cfg_sftp_attempt_gssapi_key_exchange ? BST_CHECKED : BST_UNCHECKED ), 0 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_ENABLE_COMPRESSION:
				case BTN_ATTEMPT_GSSAPI_AUTHENTICATION:
				case BTN_ATTEMPT_GSSAPI_KEY_EXCHANGE:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case EDIT_SFTP_KEEP_ALIVE_TIME:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > KEEP_ALIVE_MAX )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )KEEP_ALIVE_MAX_S );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != ( unsigned long )cfg_sftp_keep_alive_time )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_SFTP_REKEY_TIME:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > REKEY_MAX )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )REKEY_MAX_S );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != ( unsigned long )cfg_sftp_rekey_time )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_SFTP_GSS_REKEY_TIME:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > GSS_REKEY_MAX )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )GSS_REKEY_MAX_S );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != ( unsigned long )cfg_sftp_gss_rekey_time )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_SFTP_REKEY_DATA_LIMIT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num == 0xFFFFFFFF )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"4294967295" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num > 0 )
						{
							FormatSizes( rekey_data_limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
						}
						else
						{
							_wmemcpy_s( rekey_data_limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_sftp_rekey_data_limit;
						ti.lpszText = rekey_data_limit_tooltip_text;
						_SendMessageW( g_hWnd_rekey_data_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );

						if ( num != cfg_sftp_rekey_data_limit )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
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
			// Get our treeview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TVN_BEGINDRAG:
				{
					NMTREEVIEW *nmtv = ( NMTREEVIEW * )lParam;
					if ( nmtv != NULL )
					{
						g_tv_start_item = g_tv_end_item = nmtv->itemNew.hItem;
						g_hWnd_tv_current = nmtv->hdr.hwndFrom;

						*( HTREEITEM * )&g_lb_item_rc = g_tv_start_item;
						_SendMessageW( nmtv->hdr.hwndFrom, TVM_GETITEMRECT, TRUE, ( LPARAM )&g_lb_item_rc );

						_SetCapture( hWnd );

						g_tv_dragging = true;
					}
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_MOUSEMOVE:
		{
			if ( g_tv_dragging )
			{
				POINT pt;
				pt.x = GET_X_LPARAM( lParam );
				pt.y = GET_Y_LPARAM( lParam );
				_ClientToScreen( hWnd, &pt );
				_ScreenToClient( g_hWnd_tv_current, &pt );

				TVHITTESTINFO tvht;
				_memzero( &tvht, sizeof( TVHITTESTINFO ) );
				tvht.pt.x = pt.x;
				tvht.pt.y = pt.y;
				HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_tv_current, TVM_HITTEST, 0, ( LPARAM )&tvht );
				if ( hti != NULL )
				{
					g_tv_oob = false;

					*( HTREEITEM * )&g_lb_item_rc = hti;
					_SendMessageW( g_hWnd_tv_current, TVM_GETITEMRECT, TRUE, ( LPARAM )&g_lb_item_rc );
				}
				else if ( !g_tv_oob && pt.y > 0 )
				{
					g_tv_oob = true;

					int height = g_lb_item_rc.bottom - g_lb_item_rc.top;
					g_lb_item_rc.top += height;
					g_lb_item_rc.bottom += height;
				}

				RECT rc;
				_GetClientRect( g_hWnd_tv_current, &rc );
				g_lb_item_rc.left = rc.left;
				g_lb_item_rc.right = rc.right;

				HDC hDC = _GetDC( g_hWnd_tv_current );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, rc.right - rc.left, rc.bottom - rc.top );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );

				// Fill the background.
				HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
				_FillRect( hdcMem, &rc, color );
				_DeleteObject( color );

				_SendMessageW( g_hWnd_tv_current, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT ) );

				HPEN line_color;

#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_DOT, 1, dm_color_list_highlight );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_DOT, 1, ( COLORREF )_GetSysColor( COLOR_HOTLIGHT ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
				_DeleteObject( old_color );
				_MoveToEx( hdcMem, g_lb_item_rc.left, g_lb_item_rc.top, NULL );
				_LineTo( hdcMem, g_lb_item_rc.right, g_lb_item_rc.top );
				_DeleteObject( line_color );

				_BitBlt( hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0, SRCCOPY );

				_DeleteObject( hbm );

				_DeleteDC( hdcMem );
				_ReleaseDC( g_hWnd_tv_current, hDC );
			}

			return TRUE;
		}
		break;

		case WM_LBUTTONUP:
		{
			if ( g_tv_dragging ) 
			{
				POINT pt;
				pt.x = GET_X_LPARAM( lParam );
				pt.y = GET_Y_LPARAM( lParam );
				_ClientToScreen( hWnd, &pt );
				_ScreenToClient( g_hWnd_tv_current, &pt );

				TVHITTESTINFO tvht;
				_memzero( &tvht, sizeof( TVHITTESTINFO ) );
				tvht.pt.x = pt.x;
				tvht.pt.y = pt.y;
				g_tv_end_item = ( HTREEITEM )_SendMessageW( g_hWnd_tv_current, TVM_HITTEST, 0, ( LPARAM )&tvht );
				if ( g_tv_end_item != g_tv_start_item )
				{
					wchar_t tvi_start_text[ 128 ];

					TVINSERTSTRUCT tvis;
					_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

					tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
					tvis.item.stateMask = TVIS_SELECTED | TVIS_STATEIMAGEMASK;
					tvis.item.hItem = g_tv_start_item;
					tvis.item.pszText = tvi_start_text;
					tvis.item.cchTextMax = 128;
					_SendMessageW( g_hWnd_tv_current, TVM_GETITEM, 0, ( LPARAM )&tvis.item );
					_SendMessageW( g_hWnd_tv_current, TVM_DELETEITEM, 0, ( LPARAM )g_tv_start_item );

					tvis.hParent = TVI_ROOT;

					HTREEITEM hti;
					if ( g_tv_end_item != NULL )
					{
						hti = ( HTREEITEM )_SendMessageW( g_hWnd_tv_current, TVM_GETNEXTITEM, TVGN_PREVIOUS, ( LPARAM )g_tv_end_item );
						tvis.hInsertAfter = ( hti != NULL ? hti : TVI_FIRST );
					}
					else
					{
						tvis.hInsertAfter = ( pt.y > 0 ? TVI_LAST : TVI_FIRST );
					}
					hti = ( HTREEITEM )_SendMessageW( g_hWnd_tv_current, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
					_SendMessageW( g_hWnd_tv_current, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti );

					unsigned char *priority;
					if ( g_hWnd_tv_current == g_hWnd_sftp_kex_algorithm ) { priority = g_priority_kex_algorithm; }
					else if ( g_hWnd_tv_current == g_hWnd_sftp_host_key ) { priority = g_priority_host_key; }
					else if ( g_hWnd_tv_current == g_hWnd_sftp_encryption_cipher ) { priority = g_priority_encryption_cipher; }
					else { priority = NULL; }
					SetPriorityList( g_hWnd_tv_current, priority );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}

				g_tv_oob = false;

				_ReleaseCapture();

				_InvalidateRect( g_hWnd_tv_current, NULL, TRUE );

				g_hWnd_tv_current = NULL;
				g_tv_dragging = false;
			}

			return TRUE;
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
