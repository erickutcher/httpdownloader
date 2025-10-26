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

#include "lite_gdi32.h"

#include "file_operations.h"
#include "site_manager_utilities.h"
#include "sftp.h"
#include "categories.h"

#include "utilities.h"
#include "string_tables.h"

#include "options.h"

#define OPTIONS_WINDOW_WIDTH	755
#define OPTIONS_CLIENT_HEIGHT	473

#define BTN_OK					1000
#define BTN_CANCEL				1001
#define BTN_APPLY				1002

HWND g_hWnd_options = NULL;

bool options_state_changed = false;
bool override_options_state = false;

// Options Window
HWND g_hWnd_options_tree = NULL;
HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_appearance_tab = NULL;
HWND g_hWnd_connection_tab = NULL;
HWND g_hWnd_ftp_tab = NULL;
HWND g_hWnd_sftp_tab = NULL;
HWND g_hWnd_sftp_fps_tab = NULL;
HWND g_hWnd_sftp_keys_tab = NULL;
HWND g_hWnd_proxy_tab = NULL;
HWND g_hWnd_web_server_tab = NULL;
HWND g_hWnd_advanced_tab = NULL;

HWND g_hWnd_options_ok = NULL;
HWND g_hWnd_options_cancel = NULL;
HWND g_hWnd_options_apply = NULL;

UINT current_dpi_options = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_options = 0;
HFONT hFont_options = NULL;

WNDPROC TreeViewProc = NULL;

WNDPROC FocusLBProc = NULL;
WNDPROC FocusCBProc = NULL;
WNDPROC FocusComboProc = NULL;
WNDPROC FocusEditProc = NULL;
WNDPROC FocusBtnProc = NULL;

void ScrollToFocusedWindow( HWND hWnd )
{
	HWND p_hWnd = _GetParent( hWnd );
	RECT rc, p_rc;
	_GetWindowRect( hWnd, &rc );
	_GetClientRect( p_hWnd, &p_rc );

	_MapWindowPoints( HWND_DESKTOP, p_hWnd, ( LPPOINT )&rc, 2 );

	SCROLLINFO si;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	_GetScrollInfo( p_hWnd, SB_VERT, &si );

	int delta = si.nPos;

	if ( rc.bottom > p_rc.bottom )
	{
		si.nPos += ( rc.bottom - p_rc.bottom );
	}
	else if ( rc.top < p_rc.top )
	{
		si.nPos -= ( p_rc.top - rc.top );
	}

	_SetScrollPos( p_hWnd, SB_VERT, si.nPos, TRUE );

	si.fMask = SIF_POS;
	_GetScrollInfo( p_hWnd, SB_VERT, &si );

	if ( si.nPos != delta )
	{
		_ScrollWindow( p_hWnd, 0, delta - si.nPos, NULL, NULL );
	}
}

LRESULT CALLBACK FocusLBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_SETFOCUS:
		{
			ScrollToFocusedWindow( hWnd );
		}
		break;
	}

	return _CallWindowProcW( FocusLBProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK FocusCBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_SETFOCUS:
		{
			ScrollToFocusedWindow( hWnd );
		}
		break;
	}

	return _CallWindowProcW( FocusCBProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK FocusComboSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_SETFOCUS:
		{
			ScrollToFocusedWindow( hWnd );
		}
		break;
	}

	return _CallWindowProcW( FocusComboProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK FocusEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_SETFOCUS:
		{
			ScrollToFocusedWindow( hWnd );
		}
		break;
	}

	return _CallWindowProcW( FocusEditProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK FocusBtnSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_SETFOCUS:
		{
			ScrollToFocusedWindow( hWnd );
		}
		break;
	}

	return _CallWindowProcW( FocusBtnProc, hWnd, msg, wParam, lParam );
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

		case WM_GETDLGCODE:
		{
			// Don't process the tab key if we're focusing on a window with scrollbars.
			if ( wParam == VK_TAB && !( _GetKeyState( VK_SHIFT ) & 0x8000 ) )
			{
				HTREEITEM hti = NULL;

				TVITEM tvi;
				_memzero( &tvi, sizeof( TVITEM ) );
				tvi.mask = TVIF_PARAM;
				tvi.hItem = ( HTREEITEM )_SendMessageW( hWnd, TVM_GETNEXTITEM, TVGN_CARET, ( LPARAM )hti );
				_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

				HWND hWnd_next = *( HWND * )tvi.lParam;

				// We're cheating here. Ideally we'd detect which window has scrollbars, but we already know that the appearance tab does.
				if ( hWnd_next == g_hWnd_appearance_tab ||
					 hWnd_next == g_hWnd_advanced_tab )
				{
					// returning DLGC_WANTTAB will cause a beep.
					LRESULT ret = _CallWindowProcW( TreeViewProc, hWnd, msg, wParam, lParam );

					// Can't do SetFocus() here otherwise it focuses on the listview in the appearance tab instead of the window itself.
					_PostMessageW( hWnd_next, WM_PROPAGATE, 0, 0 );

					return ret;
				}
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
			current_dpi_options = __GetDpiForWindow( hWnd );
			last_dpi_options = ( current_dpi_options == current_dpi_main ? current_dpi_options : 0 );
			hFont_options = UpdateFont( current_dpi_options );

			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_options_tree = _CreateWindowExW( WS_EX_CLIENTEDGE | TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, 10, 140, 420, hWnd, NULL, NULL, NULL );


			TVINSERTSTRUCT tvis;
			_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

			tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			tvis.item.state = TVIS_SELECTED | TVIS_EXPANDED;
			tvis.item.stateMask = TVIS_SELECTED | TVIS_EXPANDED;
			tvis.item.pszText = ST_V_General;
			tvis.item.lParam = ( LPARAM )&g_hWnd_general_tab;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_FIRST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.state = TVIS_EXPANDED;
			tvis.item.pszText = ST_V_Appearance;
			tvis.item.lParam = ( LPARAM )&g_hWnd_appearance_tab;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Connection;
			tvis.item.lParam = ( LPARAM )&g_hWnd_connection_tab;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;

			HTREEITEM hti_connection = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_FTP;
			tvis.item.lParam = ( LPARAM )&g_hWnd_ftp_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_SFTP;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = TVI_LAST;

			HTREEITEM hti_sftp = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Fingerprints;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_fps_tab;

			tvis.hParent = hti_sftp;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Private_Keys;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_keys_tab;

			tvis.hParent = hti_sftp;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Proxy;
			tvis.item.lParam = ( LPARAM )&g_hWnd_proxy_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Server;
			tvis.item.lParam = ( LPARAM )&g_hWnd_web_server_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Advanced;
			tvis.item.lParam = ( LPARAM )&g_hWnd_advanced_tab;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			// WS_EX_CONTROLPARENT for tab key access.
			// These need dimensions for scrollbars.
			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 160, 10, rc.right - 170, 420, hWnd, NULL, NULL, NULL );

			g_hWnd_options_ok = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			g_hWnd_options_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_options_apply = _CreateWindowW( WC_BUTTON, ST_V_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );

			_SendMessageW( g_hWnd_options_tree, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_options_ok, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_options_cancel, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_options_apply, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			TreeViewProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_options_tree, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_options_tree, GWLP_WNDPROC, ( LONG_PTR )TreeViewSubProc );

			options_state_changed = false;
			_EnableWindow( g_hWnd_options_apply, FALSE );

			int width = _SCALE_O_( OPTIONS_WINDOW_WIDTH );

			// Accounts for differing title bar heights.
			CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
			int height = ( cs->cy - ( rc.bottom - rc.top ) ) + _SCALE_O_( OPTIONS_CLIENT_HEIGHT );	// Bottom of last window object + 10.

			HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );

			_SetFocus( g_hWnd_options_tree );

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				_EnumChildWindows( hWnd, EnumChildProc, NULL );
				_EnumThreadWindows( GetCurrentThreadId(), EnumTLWProc, NULL );
			}
#endif

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 14 );
			_DeferWindowPos( hdwp, g_hWnd_options_tree, HWND_TOP, _SCALE_O_( 10 ), _SCALE_O_( 10 ), _SCALE_O_( 140 ), _SCALE_O_( 420 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_general_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_appearance_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_connection_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ftp_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sftp_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sftp_fps_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sftp_keys_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_proxy_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_web_server_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_advanced_tab, HWND_TOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_options_ok, HWND_TOP, rc.right - _SCALE_O_( 260 ), _SCALE_O_( 440 ), _SCALE_O_( 80 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_options_cancel, HWND_TOP, rc.right - _SCALE_O_( 175 ), _SCALE_O_( 440 ), _SCALE_O_( 80 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_options_apply, HWND_TOP, rc.right - _SCALE_O_( 90 ), _SCALE_O_( 440 ), _SCALE_O_( 80 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_options;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_options;
			current_dpi_options = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_options );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_options );
			hFont_options = hFont;

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_options == 0 )
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

			last_dpi_options = last_dpi;

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

					HWND hWnd_options_tab = *( ( HWND * )nmtv->itemNew.lParam );
					if ( hWnd_options_tab == NULL )
					{
						RECT rc;
						_GetClientRect( hWnd, &rc );

						// The creation of the windows below could trigger WM_OPTIONS_CHANGED and we want to prevent that.
						override_options_state = true;

						if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_appearance_tab ) {
							g_hWnd_appearance_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_appearance_tab", NULL, WS_VSCROLL | WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_connection_tab ) {
							g_hWnd_connection_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_connection_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_ftp_tab ) {
							g_hWnd_ftp_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_ftp_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_sftp_tab ) {
							g_hWnd_sftp_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_sftp_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_sftp_fps_tab ) {
							g_hWnd_sftp_fps_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_sftp_fps_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_sftp_keys_tab ) {
							g_hWnd_sftp_keys_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_sftp_keys_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_proxy_tab ) {
							g_hWnd_proxy_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_proxy_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_web_server_tab ) {
							g_hWnd_web_server_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_web_server_tab", NULL, WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }
						else if ( nmtv->itemNew.lParam == ( LPARAM )&g_hWnd_advanced_tab ) {
							g_hWnd_advanced_tab = hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"class_advanced_tab", NULL, WS_VSCROLL | WS_CHILD | WS_TABSTOP, _SCALE_O_( 160 ), _SCALE_O_( 10 ), rc.right - _SCALE_O_( 170 ), _SCALE_O_( 420 ), hWnd, NULL, NULL, NULL ); }

						override_options_state = false;

#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							_EnumChildWindows( hWnd_options_tab, EnumChildProc, NULL );
							EnumChildProc( hWnd_options_tab, NULL );
							_EnumThreadWindows( GetCurrentThreadId(), EnumTLWProc, NULL );
						}
#endif
					}

					_ShowWindow( hWnd_options_tab, SW_SHOW );
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

					if ( g_hWnd_general_tab != NULL ) { _SendMessageW( g_hWnd_general_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_appearance_tab != NULL ) { _SendMessageW( g_hWnd_appearance_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_connection_tab != NULL ) { _SendMessageW( g_hWnd_connection_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_ftp_tab != NULL ) { _SendMessageW( g_hWnd_ftp_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_sftp_tab != NULL ) { _SendMessageW( g_hWnd_sftp_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_proxy_tab != NULL ) { _SendMessageW( g_hWnd_proxy_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_web_server_tab != NULL ) { _SendMessageW( g_hWnd_web_server_tab, WM_SAVE_OPTIONS, 0, 0 ); }
					if ( g_hWnd_advanced_tab != NULL ) { _SendMessageW( g_hWnd_advanced_tab, WM_SAVE_OPTIONS, 0, 0 ); }

					save_config();

					if ( site_list_changed ) { save_site_info(); site_list_changed = false; }
					if ( sftp_fps_host_list_changed ) { save_sftp_fps_host_info(); sftp_fps_host_list_changed = false; }
					if ( sftp_keys_host_list_changed ) { save_sftp_keys_host_info(); sftp_keys_host_list_changed = false; }
					if ( category_list_changed ) { save_category_info(); category_list_changed = false; }

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
		break;

		case WM_OPTIONS_CHANGED:
		{
			if ( !override_options_state )
			{
				options_state_changed = true;
				_EnableWindow( g_hWnd_options_apply, ( BOOL )wParam );
			}

			return 0;
		}
		break;

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
			// Delete our font.
			_DeleteObject( hFont_options );

			g_hWnd_appearance_tab = NULL;
			g_hWnd_connection_tab = NULL;
			g_hWnd_ftp_tab = NULL;
			g_hWnd_sftp_tab = NULL;
			g_hWnd_sftp_fps_tab = NULL;
			g_hWnd_sftp_keys_tab = NULL;
			g_hWnd_proxy_tab = NULL;
			g_hWnd_web_server_tab = NULL;
			g_hWnd_advanced_tab = NULL;

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
