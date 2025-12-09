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

#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_comctl32.h"
#include "folder_browser.h"
#include "file_operations.h"
#include "utilities.h"
#include "connection.h"

#include "treelistview.h"
#include "list_operations.h"

#include "dark_mode.h"

#include "string_tables.h"

#include "categories.h"

#define ADD_CATEGORY_WINDOW_WIDTH	430
#define ADD_CATEGORY_CLIENT_HEIGHT	196

#define EDIT_ADD_CATEGORY_CATEGORY		1000
#define BTN_LOAD_CATEGORY_DIRECTORY		1001
#define BTN_ADD_CATEGORY_ADD			1002
#define BTN_ADD_CATEGORY_CANCEL			1003

HWND g_hWnd_static_category = NULL;
HWND g_hWnd_add_category_category = NULL;

HWND g_hWnd_static_category_file_extensions = NULL;
HWND g_hWnd_category_file_extensions = NULL;

HWND g_hWnd_static_category_directory = NULL;
HWND g_hWnd_category_directory = NULL;
HWND g_hWnd_load_category_directory = NULL;

HWND g_hWnd_add_category_add = NULL;
HWND g_hWnd_add_category_cancel = NULL;

HWND g_hWnd_add_category = NULL;

WNDPROC CategoriesProc = NULL;
WNDPROC CategoriesEditProc = NULL;

HTREEITEM g_hti_categories = NULL;

CATEGORY_TREE_INFO *g_drag_and_drop_cti = NULL;

HMENU g_hMenuSub_categories_add = NULL;
HMENU g_hMenuSub_categories_update_remove = NULL;

DoublyLinkedList *g_treeview_list = NULL;
DoublyLinkedList *g_category_list = NULL;	// This is part of g_treeview_list. It doesn't contain the All and Status items.
bool category_list_changed = false;

bool g_update_add_category_window = true;
bool g_update_update_category_window = true;
bool g_update_site_manager_category_window = true;

dllrbt_tree *g_shared_categories = NULL;
dllrbt_tree *g_category_file_extensions = NULL;

//

wchar_t treeview_tooltip_text[ MAX_PATH + MAX_PATH + 1 ];
HWND g_hWnd_treeview_tooltip = NULL;
BOOL g_tv_is_tracking = FALSE;
int g_tv_tracking_x = -1;
int g_tv_tracking_y = -1;

//

HTREEITEM g_selected_hti = NULL;
DoublyLinkedList *g_selected_category_info_dll_node = NULL;
CATEGORY_INFO_ *g_selected_category_info = NULL;
wchar_t *t_category_download_directory = NULL;

//

UINT current_dpi_add_category = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_add_category = 0;
HFONT hFont_add_category = NULL;

#define _SCALE_AC_( x )						_SCALE_( ( x ), dpi_add_category )

LRESULT CALLBACK AddCategoryWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			current_dpi_add_category = __GetDpiForWindow( hWnd );
			last_dpi_add_category = ( current_dpi_add_category == current_dpi_main ? current_dpi_add_category : 0 );
			hFont_add_category = UpdateFont( current_dpi_add_category );

			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_static_category = _CreateWindowW( WC_STATIC, ST_V_Category_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_category_category = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_CATEGORY_CATEGORY, NULL, NULL );

			g_hWnd_static_category_file_extensions = _CreateWindowW( WC_STATIC, ST_V_Associate_file_extension_s__with_category_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_category_file_extensions = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_category_directory = _CreateWindowW( WC_STATIC, ST_V_Download_directory_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_category_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_load_category_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_LOAD_CATEGORY_DIRECTORY, NULL, NULL );

			_SendMessageW( g_hWnd_add_category_category, EM_LIMITTEXT, MAX_PATH - 1, 0 );

			g_hWnd_add_category_add = _CreateWindowW( WC_BUTTON, ST_V_Add, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_CATEGORY_ADD, NULL, NULL );
			g_hWnd_add_category_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_CATEGORY_CANCEL, NULL, NULL );

			_SendMessageW( g_hWnd_static_category, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_add_category_category, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_static_category_file_extensions, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_category_file_extensions, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_static_category_directory, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_category_directory, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_load_category_directory, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_add_category_add, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );
			_SendMessageW( g_hWnd_add_category_cancel, WM_SETFONT, ( WPARAM )hFont_add_category, 0 );

			//_SetFocus( g_hWnd_add_category_category );

			int width = ADD_CATEGORY_WINDOW_WIDTH;

			// Accounts for differing title bar heights.
			CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
			int height = ( cs->cy - ( rc.bottom - rc.top ) ) + ADD_CATEGORY_CLIENT_HEIGHT;	// Bottom of last window object + 10.

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

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_ADD_CATEGORY_ADD:
				{
					CATEGORY_UPDATE_INFO *cui = ( CATEGORY_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_UPDATE_INFO ) );
					if ( cui != NULL )
					{
						cui->old_ci = g_selected_category_info;
						if ( g_selected_category_info != NULL )
						{
							cui->update_type = 1;	// Update
						}
						else
						{
							cui->update_type = 0;	// Add
						}
						cui->hti = g_selected_hti;

						CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )GlobalAlloc( GPTR, sizeof( CATEGORY_INFO_ ) );
						if ( ci != NULL )
						{
							unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_add_category_category, WM_GETTEXTLENGTH, 0, 0 );

							ci->category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );	// Include the NULL terminator.
							_SendMessageW( g_hWnd_add_category_category, WM_GETTEXT, edit_length + 1, ( LPARAM )ci->category );

							edit_length = ( unsigned int )_SendMessageW( g_hWnd_category_file_extensions, WM_GETTEXTLENGTH, 0, 0 );

							ci->file_extensions = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );	// Include the NULL terminator.
							_SendMessageW( g_hWnd_category_file_extensions, WM_GETTEXT, edit_length + 1, ( LPARAM )ci->file_extensions );

							ci->download_directory = t_category_download_directory;
							t_category_download_directory = NULL;

							cui->ci = ci;

							// cui is freed in handle_category_list.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_category_list, ( void * )cui, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								FreeCategoryInfo( &cui->ci );
								GlobalFree( cui );
							}
						}
						else
						{
							GlobalFree( cui );
						}
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_ADD_CATEGORY_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_ADD_CATEGORY_CATEGORY:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						if ( _SendMessageW( g_hWnd_add_category_category, WM_GETTEXTLENGTH, 0, 0 ) > 0 &&
							 _SendMessageW( g_hWnd_category_directory, WM_GETTEXTLENGTH, 0, 0 ) > 0 )
						{
							_EnableWindow( g_hWnd_add_category_add, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_add_category_add, FALSE );
						}
					}
				}
				break;

				case BTN_LOAD_CATEGORY_DIRECTORY:
				{
					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_category_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_category_download_directory );
						t_category_download_directory = directory;

						_SendMessageW( g_hWnd_category_directory, WM_SETTEXT, 0, ( LPARAM )t_category_download_directory );

						if ( _SendMessageW( g_hWnd_add_category_category, WM_GETTEXTLENGTH, 0, 0 ) > 0 )
						{
							_EnableWindow( g_hWnd_add_category_add, TRUE );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HDWP hdwp = _BeginDeferWindowPos( 9 );
			_DeferWindowPos( hdwp, g_hWnd_static_category, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 10 ), rc.right - _SCALE_AC_( 20 ), _SCALE_AC_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_category_category, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 28 ), rc.right - _SCALE_AC_( 20 ), _SCALE_AC_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_category_file_extensions, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 61 ), rc.right - _SCALE_AC_( 20 ), _SCALE_AC_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_category_file_extensions, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 79 ), rc.right - _SCALE_AC_( 20 ), _SCALE_AC_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_category_directory, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 112 ), rc.right - _SCALE_AC_( 20 ), _SCALE_AC_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_category_directory, HWND_TOP, _SCALE_AC_( 10 ), _SCALE_AC_( 130 ), rc.right - _SCALE_AC_( 60 ), _SCALE_AC_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_load_category_directory, HWND_TOP, rc.right - _SCALE_AC_( 45 ), _SCALE_AC_( 130 ), _SCALE_AC_( 35 ), _SCALE_AC_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_add_category_add, HWND_TOP, rc.right - _SCALE_AC_( 175 ), _SCALE_AC_( 163 ), _SCALE_AC_( 80 ), _SCALE_AC_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_category_cancel, HWND_TOP, rc.right - _SCALE_AC_( 90 ), _SCALE_AC_( 163 ), _SCALE_AC_( 80 ), _SCALE_AC_( 23 ), SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_add_category;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_add_category;
			current_dpi_add_category = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_add_category );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_add_category );
			hFont_add_category = hFont;

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_add_category == 0 )
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

			last_dpi_add_category = last_dpi;

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			g_selected_hti = ( HTREEITEM )lParam;
			g_selected_category_info_dll_node = NULL;
			g_selected_category_info = NULL;

			if ( t_category_download_directory != NULL )
			{
				GlobalFree( t_category_download_directory );
				t_category_download_directory = NULL;
			}

			TVITEM tvi;
			_memzero( &tvi, sizeof( TVITEM ) );
			tvi.mask = TVIF_PARAM;
			tvi.hItem = g_selected_hti;
			_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvi );

			CATEGORY_TREE_INFO *cti = NULL;
			DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
			if ( dll_node != NULL && dll_node->data != NULL )
			{
				cti = ( CATEGORY_TREE_INFO * )dll_node->data;
			}

			if ( wParam == TRUE && cti != NULL && cti->data != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO )
			{
				g_selected_category_info_dll_node = dll_node;
				g_selected_category_info = ( CATEGORY_INFO_ * )cti->data;

				t_category_download_directory = GlobalStrDupW( g_selected_category_info->download_directory );

				_SendMessageW( g_hWnd_add_category_category, WM_SETTEXT, 0, ( LPARAM )g_selected_category_info->category );
				_SendMessageW( g_hWnd_category_file_extensions, WM_SETTEXT, 0, ( LPARAM )g_selected_category_info->file_extensions );
				_SendMessageW( g_hWnd_category_directory, WM_SETTEXT, 0, ( LPARAM )t_category_download_directory );

				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_V_Update_Category );
				_SendMessageW( g_hWnd_add_category_add, WM_SETTEXT, 0, ( LPARAM )ST_V_Update );

				_EnableWindow( g_hWnd_add_category_add, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_add_category_category, WM_SETTEXT, 0, NULL );
				_SendMessageW( g_hWnd_category_file_extensions, WM_SETTEXT, 0, NULL );
				_SendMessageW( g_hWnd_category_directory, WM_SETTEXT, 0, NULL );

				_SendMessageW( hWnd, WM_SETTEXT, 0, ( LPARAM )ST_V_Add_Category );
				_SendMessageW( g_hWnd_add_category_add, WM_SETTEXT, 0, ( LPARAM )ST_V_Add );
			}

			_SendMessageA( g_hWnd_add_category_category, EM_SETSEL, 0, -1 );
			_SetFocus( g_hWnd_add_category_category );

			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );

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
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			// Delete our font.
			_DeleteObject( hFont_add_category );

			if ( t_category_download_directory != NULL )
			{
				GlobalFree( t_category_download_directory );
				t_category_download_directory = NULL;
			}

			g_hWnd_add_category = NULL;

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


LRESULT CALLBACK CategoriesSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_GETDLGCODE:
		{
			// Don't process the tab key if we're focusing on a window with scrollbars.
			if ( wParam == VK_TAB && !( _GetKeyState( VK_SHIFT ) & 0x8000 ) )
			{
				// returning DLGC_WANTTAB will cause a beep.
				LRESULT ret = _CallWindowProcW( CategoriesProc, hWnd, msg, wParam, lParam );

				_SetFocus( g_hWnd_tlv_files );

				return ret;
			}
		}
		break;

		case WM_MOUSEHOVER:
		{
			if ( g_tv_is_tracking == TRUE )
			{
				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;

				tti.lpszText = treeview_tooltip_text;

				_SendMessageW( g_hWnd_treeview_tooltip, TTM_SETTOOLINFO, 0, ( LPARAM )&tti );
				_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, TRUE, ( LPARAM )&tti );
			}
		}
		break;

		case WM_MOUSELEAVE:
		{
			if ( g_tv_is_tracking == TRUE )
			{
				g_tv_is_tracking = FALSE;

				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;
				//tti.lpszText = NULL;
				_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			int tracking_x = GET_X_LPARAM( lParam );
			int tracking_y = GET_Y_LPARAM( lParam );

			TVHITTESTINFO tvht;
			_memzero( &tvht, sizeof( TVHITTESTINFO ) );
			tvht.pt.x = tracking_x;
			tvht.pt.y = tracking_y;
			HTREEITEM hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_HITTEST, 0, ( LPARAM )&tvht );
			if ( hti != NULL )
			{
				if ( tracking_x != g_tv_tracking_x || tracking_y != g_tv_tracking_y )
				{
					g_tv_tracking_x = tracking_x;
					g_tv_tracking_y = tracking_y;

					TVITEM tvi;
					_memzero( &tvi, sizeof( TVITEM ) );
					tvi.mask = TVIF_PARAM;
					tvi.hItem = hti;
					_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

					CATEGORY_TREE_INFO *cti = NULL;
					DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
					if ( dll_node != NULL )
					{
						cti = ( CATEGORY_TREE_INFO * )dll_node->data;
					}

					if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
					{
						CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;
						__snwprintf( treeview_tooltip_text, MAX_PATH + MAX_PATH + 1, L"%s\r\n%s", ci->category, ci->download_directory );

						TOOLINFO tti;
						_memzero( &tti, sizeof( TOOLINFO ) );
						tti.cbSize = sizeof( TOOLINFO );
						tti.hwnd = hWnd;
						tti.uId = ( UINT_PTR )hWnd;
						_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );

						TRACKMOUSEEVENT tmi;
						tmi.cbSize = sizeof( TRACKMOUSEEVENT );
						tmi.dwFlags = TME_HOVER | TME_LEAVE;
						tmi.hwndTrack = hWnd;
						tmi.dwHoverTime = 2000;
						g_tv_is_tracking = _TrackMouseEvent( &tmi );
					}
					else
					{
						RECT rc, rc2;

						_GetClientRect( hWnd, &rc );

						*( HTREEITEM * )&rc2 = hti;
						_SendMessageW( hWnd, TVM_GETITEMRECT, TRUE, ( LPARAM )&rc2 );
					
						if ( rc2.right > rc.right )
						{
							tvi.mask = TVIF_TEXT;
							tvi.hItem = hti;
							tvi.pszText = treeview_tooltip_text;
							tvi.cchTextMax = MAX_PATH + MAX_PATH + 1;
							_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

							TOOLINFO tti;
							_memzero( &tti, sizeof( TOOLINFO ) );
							tti.cbSize = sizeof( TOOLINFO );
							tti.hwnd = hWnd;
							tti.uId = ( UINT_PTR )hWnd;
							_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );

							TRACKMOUSEEVENT tmi;
							tmi.cbSize = sizeof( TRACKMOUSEEVENT );
							tmi.dwFlags = TME_HOVER | TME_LEAVE;
							tmi.hwndTrack = hWnd;
							tmi.dwHoverTime = HOVER_DEFAULT;
							g_tv_is_tracking = _TrackMouseEvent( &tmi );
						}
						else
						{
							g_tv_is_tracking = FALSE;

							TOOLINFO tti;
							_memzero( &tti, sizeof( TOOLINFO ) );
							tti.cbSize = sizeof( TOOLINFO );
							tti.hwnd = hWnd;
							tti.uId = ( UINT_PTR )hWnd;
							//tti.lpszText = NULL;
							_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
						}
					}
				}
			}
			else// if ( _SendMessageW( g_hWnd_treeview_tooltip, TTM_GETCURRENTTOOL, 0, NULL ) != 0 )
			{
				g_tv_is_tracking = FALSE;

				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;
				//tti.lpszText = NULL;
				_SendMessageW( g_hWnd_treeview_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
			}
		}
		break;

		// The treeview steals focus after a double click. Why?
		case WM_LBUTTONDBLCLK:
		{
			TVHITTESTINFO tvht;
			_memzero( &tvht, sizeof( TVHITTESTINFO ) );
			tvht.pt.x = GET_X_LPARAM( lParam );
			tvht.pt.y = GET_Y_LPARAM( lParam );
			HTREEITEM hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_HITTEST, 0, ( LPARAM )&tvht );
			if ( hti != NULL && hti == ( HTREEITEM )_SendMessageW( hWnd, TVM_GETNEXTITEM, TVGN_CARET, NULL ) )
			{
				TVITEM tvi;
				_memzero( &tvi, sizeof( TVITEM ) );
				tvi.mask = TVIF_PARAM;
				tvi.hItem = hti;
				_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

				DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
				if ( dll_node != NULL )
				{
					CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
					if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
					{
						if ( ( GetKeyState( VK_CONTROL ) & 0x8000 ) &&
							 ( GetKeyState( VK_SHIFT ) & 0x8000 ) )
						{
							_SendMessageW( g_hWnd_main, WM_COMMAND, MENU_CAT_OPEN, 0 );
						}
						else
						{
							_SendMessageW( g_hWnd_main, WM_COMMAND, MENU_CAT_UPDATE, 0 );
						}

						return 0;
					}
				}
			}
		}
		break;

		case WM_KEYDOWN:
		{
			switch ( wParam )
			{
				case VK_APPS:	// Context menu key.
				{
					POINT p;
					p.x = 0;
					p.y = 0;

					HTREEITEM hti = ( HTREEITEM )_SendMessageW( hWnd, TVM_GETNEXTITEM, TVGN_CARET, NULL );
					if ( hti != NULL )
					{
						RECT rc;
						*( HTREEITEM * )&rc = hti;
						_SendMessageW( hWnd, TVM_GETITEMRECT, FALSE, ( LPARAM )&rc );

						p.x = rc.left + ( ( rc.bottom - rc.top ) / 2 );
						p.y = p.x + rc.top + ( ( rc.bottom - rc.top ) / 2 );
						_ClientToScreen( hWnd, &p );

						TVITEM tvi;
						_memzero( &tvi, sizeof( TVITEM ) );
						tvi.mask = TVIF_PARAM;
						tvi.hItem = hti;
						_SendMessageW( hWnd, TVM_GETITEM, 0, ( LPARAM )&tvi );

						DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
						if ( dll_node != NULL )
						{
							CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
							if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO )
							{
								if ( cti->data != NULL )	// Update/Remove
								{
									_TrackPopupMenu( g_hMenuSub_categories_update_remove, 0, p.x, p.y, 0, _GetParent( hWnd ), NULL );
								}
								else	// Add
								{
									_TrackPopupMenu( g_hMenuSub_categories_add, 0, p.x, p.y, 0, _GetParent( hWnd ), NULL );
								}
							}
						}
					}
					else
					{
						_TrackPopupMenu( g_hMenuSub_categories_add, 0, p.x, p.y, 0, _GetParent( hWnd ), NULL );
					}
				}
				break;
			}
		}
		break;

		case WM_NCCALCSIZE:
		{
			// Draw our scrollbars if there's any.
			LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

			if ( cfg_show_toolbar )
			{
				if ( wParam == FALSE )
				{
					RECT *pRect = ( RECT * )lParam;
					++pRect->top;
				}
				else// if ( wParam == TRUE )
				{
					NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
					++nccsp->rgrc[ 0 ].top;
				}
			}

			if ( cfg_show_categories )
			{
				if ( wParam == FALSE )
				{
					RECT *pRect = ( RECT * )lParam;
					--pRect->right;
				}
				else// if ( wParam == TRUE )
				{
					NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
					--nccsp->rgrc[ 0 ].right;
				}
			}

			return ret;
		}
		break;

		case WM_NCPAINT:
		{
			// Draw our scrollbars if there's any.
			LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

			if ( cfg_show_toolbar )
			{
				RECT rc;
				_GetWindowRect( hWnd, &rc );

				HDC hDC = _GetWindowDC( hWnd );
				HPEN line_color;
#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_SOLID, 1, ( COLORREF )_GetSysColor( COLOR_3DSHADOW ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hDC, line_color );
				_DeleteObject( old_color );

				_MoveToEx( hDC, 0, 0, NULL );
				_LineTo( hDC, rc.right - rc.left, 0 );
				_DeleteObject( line_color );

				_ReleaseDC( hWnd, hDC );
			}

			if ( cfg_show_categories )
			{
				RECT rc;
				_GetWindowRect( hWnd, &rc );

				HDC hDC = _GetWindowDC( hWnd );
				HPEN line_color;
#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_SOLID, 1, ( COLORREF )_GetSysColor( COLOR_3DSHADOW ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hDC, line_color );
				_DeleteObject( old_color );

				_MoveToEx( hDC, ( rc.right - rc.left ) - 1, 0, NULL );
				_LineTo( hDC, ( rc.right - rc.left ) - 1, rc.bottom - rc.top );
				_DeleteObject( line_color );

				_ReleaseDC( hWnd, hDC );
			}

			return ret;
		}
		break;
	}

	return _CallWindowProcW( CategoriesProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK CategoriesEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_GETDLGCODE:
		{
			if ( wParam == VK_RETURN || wParam == VK_ESCAPE )
			{
				return DLGC_WANTALLKEYS;
			}
		}
		break;
	}

	return _CallWindowProcW( CategoriesEditProc, hWnd, msg, wParam, lParam );
}

void CreateCategoryTreeView( HWND hWnd_categories )
{
	g_hWnd_treeview_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd_categories, NULL, NULL, NULL );
	treeview_tooltip_text[ 0 ] = 0;

	TOOLINFO tti;
	_memzero( &tti, sizeof( TOOLINFO ) );
	tti.cbSize = sizeof( TOOLINFO );
	tti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	tti.hwnd = hWnd_categories;
	tti.uId = ( UINT_PTR )hWnd_categories;

	_SendMessageW( g_hWnd_treeview_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
	_SendMessageW( g_hWnd_treeview_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );

	CategoriesProc = ( WNDPROC )_GetWindowLongPtrW( hWnd_categories, GWLP_WNDPROC );
	_SetWindowLongPtrW( hWnd_categories, GWLP_WNDPROC, ( LONG_PTR )CategoriesSubProc );

	//

	TVINSERTSTRUCT tvis;
	_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

	CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_NONE;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	g_treeview_list = DLL_CreateNode( ( void * )cti );

	tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	tvis.item.state = TVIS_SELECTED | TVIS_EXPANDED;
	tvis.item.stateMask = TVIS_SELECTED | TVIS_EXPANDED;
	tvis.item.pszText = ST_V_All;
	tvis.item.lParam = ( LPARAM )g_treeview_list;

	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_FIRST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Status;
	tvis.item.lParam = ( LPARAM )NULL;

	tvis.hParent = NULL;
	tvis.hInsertAfter = TVI_LAST;

	HTREEITEM hti_status = ( HTREEITEM )_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_DOWNLOADING;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Downloading;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_PAUSED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Paused;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_STOPPED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Stopped;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_COMPLETED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Completed;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_SKIPPED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Skipped;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_QUEUED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Queued;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_FAILED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Failed;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	tvis.item.state = 0;
	tvis.item.pszText = ST_V_Other;
	tvis.item.lParam = ( LPARAM )NULL;

	tvis.hParent = hti_status;
	tvis.hInsertAfter = TVI_LAST;

	HTREEITEM hti_other = ( HTREEITEM )_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_CONNECTING;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Connecting;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_TIMED_OUT;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Timed_Out;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_RESTART;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Restarting;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_AUTH_REQUIRED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Authorization_Required;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_PROXY_AUTH_REQUIRED;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Proxy_Authentication_Required;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_ALLOCATING_FILE;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Allocating_File;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_MOVING_FILE;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Moving_File;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_INSUFFICIENT_DISK_SPACE;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Insufficient_Disk_Space;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = ( void * )STATUS_FILE_IO_ERROR;
	cti->type = CATEGORY_TREE_INFO_TYPE_STATUS;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_File_IO_Error;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = hti_other;
	tvis.hInsertAfter = TVI_LAST;

	_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
	cti->data = NULL;
	cti->type = CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO;
	dll_node = DLL_CreateNode( ( void * )cti );
	DLL_AddNode( &g_treeview_list, dll_node, -1 );

	tvis.item.state = TVIS_EXPANDED;
	tvis.item.pszText = ST_V_Categories;
	tvis.item.lParam = ( LPARAM )dll_node;

	tvis.hParent = NULL;
	tvis.hInsertAfter = TVI_LAST;//hti_status;

	g_hti_categories = ( HTREEITEM )_SendMessageW( hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

	//

	// Merge the two lists with the category list at the end.
	// dll_node is the last node in the treeview list. It was set above.
	if ( g_category_list != NULL )
	{
		if ( g_category_list->prev != NULL )
		{
			g_treeview_list->prev = g_category_list->prev;
		}
		else
		{
			g_treeview_list->prev = g_category_list;
		}
		dll_node->next = g_category_list;
		g_category_list->prev = dll_node;
	}

	//

	g_hMenuSub_categories_add = _CreatePopupMenu();

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Add_Category___;
	mii.cch = ST_L_Add_Category___;
	mii.wID = MENU_CAT_ADD;
	_InsertMenuItemW( g_hMenuSub_categories_add, 0, TRUE, &mii );

	g_hMenuSub_categories_update_remove = _CreatePopupMenu();

	mii.dwTypeData = ST_V_Open_Directory;
	mii.cch = ST_L_Open_Directory;
	mii.wID = MENU_CAT_OPEN;
	_InsertMenuItemW( g_hMenuSub_categories_update_remove, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_categories_update_remove, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Update_Category___;
	mii.cch = ST_L_Update_Category___;
	mii.wID = MENU_CAT_UPDATE;
	_InsertMenuItemW( g_hMenuSub_categories_update_remove, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_categories_update_remove, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Remove;
	mii.cch = ST_L_Remove;
	mii.wID = MENU_CAT_REMOVE;
	_InsertMenuItemW( g_hMenuSub_categories_update_remove, 4, TRUE, &mii );
}

void CleanupCategoryList()
{
	DoublyLinkedList *dll_node = g_treeview_list;

	while ( dll_node != NULL )
	{
		DoublyLinkedList *del_node = dll_node;
		dll_node = dll_node->next;

		GlobalFree( del_node->data );	// This is a CATEGORY_TREE_INFO structure. Any allocated data inside it will be in the g_category_info tree and cleaned up in main().
		GlobalFree( del_node );
	}

	g_treeview_list = NULL;
}

void FreeCategoryInfo( CATEGORY_INFO_ **category_info )
{
	if ( *category_info != NULL )
	{
		if ( ( *category_info )->category != NULL ) { GlobalFree( ( *category_info )->category ); }
		if ( ( *category_info )->file_extensions != NULL ) { GlobalFree( ( *category_info )->file_extensions ); }
		if ( ( *category_info )->download_directory != NULL ) { GlobalFree( ( *category_info )->download_directory ); }

		GlobalFree( *category_info );

		*category_info = NULL;
	}
}

int dllrbt_compare_category_info( void *a, void *b )
{
	CATEGORY_INFO_ *a1 = ( CATEGORY_INFO_ * )a;
	CATEGORY_INFO_ *b1 = ( CATEGORY_INFO_ * )b;

	if ( a1 == b1 )
	{
		return 0;
	}

	return lstrcmpW( a1->category, b1->category );
}

char read_category_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\categories\0", 12 );
	//g_base_directory[ g_base_directory_length + 11 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading categories: %S", g_base_directory );
#endif

	HANDLE hFile_read = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_read = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_read, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		wchar_t	*category;
		wchar_t *file_extensions;
		wchar_t	*download_directory;

		//

		unsigned char magic_identifier[ 4 ];
		BOOL bRet = ReadFile( hFile_read, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
		if ( bRet != FALSE )
		{
#ifdef ENABLE_LOGGING
			lfz += 4;
#endif
			unsigned char version = magic_identifier[ 3 ] - 0x50;

			if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_CATEGORIES, 3 ) == 0 && version <= 0x0F )
			{
				char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 3 ) );	// 512 KB buffer.
				if ( buf != NULL )
				{
					DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

					while ( total_read < fz )
					{
						bRet = ReadFile( hFile_read, buf, sizeof( char ) * 524288, &read, NULL );
						if ( bRet == FALSE )
						{
							break;
						}

#ifdef ENABLE_LOGGING
						lfz += read;
#endif

						buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

						// This terminates wide character strings so we don't read past the buffer.
						buf[ read + 1 ] = 0;
						buf[ read + 2 ] = 0;

						total_read += read;

						// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
						// If it's larger than our buffer, then the file is probably invalid/corrupt.
						if ( total_read == last_total )
						{
							break;
						}

						last_total = total_read;

						p = buf;
						offset = last_entry = 0;

						while ( offset < read )
						{
							category = NULL;
							file_extensions = NULL;
							download_directory = NULL;

							// Category
							int string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( category, string_length, p, string_length );
							*( category + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// File Extensions
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset > read ) { goto CLEANUP; }

							file_extensions = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( file_extensions, string_length, p, string_length );
							*( file_extensions + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// Download Directory
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset > read ) { goto CLEANUP; }

							download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( download_directory, string_length, p, string_length );
							*( download_directory + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							//

							last_entry = offset;	// This value is the ending offset of the last valid entry.

							CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_INFO_ ) );
							if ( ci != NULL )
							{
								ci->category = category;
								ci->file_extensions = file_extensions;
								ci->download_directory = download_directory;

								if ( dllrbt_insert( g_category_info, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
								{
									FreeCategoryInfo( &ci );
								}
								else
								{
									CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
									cti->data = ( void * )ci;
									cti->type = CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO;
									DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )cti );

									DLL_AddNode( &g_category_list, dll_node, -1 );

									CacheCategory( ci->category );

									//

									wchar_t *file_extension_start = file_extensions;
									wchar_t *file_extension_end = file_extensions;

									for ( ;; )
									{
										if ( *file_extension_end == 0 || *file_extension_end == L';' || *file_extension_end == L',' )
										{
											wchar_t tmp_end = *file_extension_end;
											*file_extension_end = NULL;

											int file_extension_length = ( int )( file_extension_end - file_extension_start );
											if ( file_extension_length > 0 )
											{
												CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_FILE_EXTENSION_INFO ) );
												cfei->ci = ci;

												cfei->file_extension = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( file_extension_length + 1 ) );
												_wmemcpy_s( cfei->file_extension, file_extension_length + 1, file_extension_start, file_extension_length );
												cfei->file_extension[ file_extension_length ] = 0;	// Sanity.

												if ( dllrbt_insert( g_category_file_extensions, ( void * )cfei->file_extension, ( void * )cfei ) != DLLRBT_STATUS_OK )
												{
													// Already exits.
													GlobalFree( cfei->file_extension );
													GlobalFree( cfei );
												}
											}

											if ( tmp_end == 0 )
											{
												break;
											}

											file_extension_start = file_extension_end + 1;
											*file_extension_end = tmp_end;	// Restore.
										}

										++file_extension_end;
									}
								}
							}
							else
							{
								GlobalFree( category );
								GlobalFree( file_extensions );
								GlobalFree( download_directory );
							}

							continue;

			CLEANUP:
							GlobalFree( category );
							GlobalFree( file_extensions );
							GlobalFree( download_directory );

							// Go back to the last valid entry.
							if ( total_read < fz )
							{
								total_read -= ( read - last_entry );
								SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
							}

							break;
						}
					}

					GlobalFree( buf );
				}
			}
			else
			{
				ret_status = -2;	// Bad file format.
			}
		}
		else
		{
			ret_status = -1;	// Can't open file for reading.
		}

		UnlockFileEx( hFile_read, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_read );	
	}
	else
	{
		DWORD gle = GetLastError();
		if ( gle == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}
		else if ( gle == ERROR_FILE_NOT_FOUND )
		{
			/*wchar_t *folder_paths[ 6 ] = { NULL };*/
			wchar_t *folder_paths[ 6 ];
			_memzero( folder_paths, sizeof( wchar_t * ) * 6 );

			/*const GUID *guids[ 6 ] = { &FOLDERID_Desktop,
									   &FOLDERID_Documents,
									   &FOLDERID_Downloads,
									   &FOLDERID_Music,
									   &FOLDERID_Pictures,
									   &FOLDERID_Videos };*/
			const GUID *guids[ 6 ];
			guids[ 0 ] = &FOLDERID_Desktop;
			guids[ 1 ] = &FOLDERID_Documents;
			guids[ 2 ] = &FOLDERID_Downloads;
			guids[ 3 ] = &FOLDERID_Music;
			guids[ 4 ] = &FOLDERID_Pictures;
			guids[ 5 ] = &FOLDERID_Videos;

			// Saves into C:\Users\[USER]\Downloads
			if ( shell32_state == SHELL32_STATE_RUNNING )
			{
				_SHGetKnownFolderPath = ( pSHGetKnownFolderPath )GetProcAddress( hModule_shell32, "SHGetKnownFolderPath" );
				if ( _SHGetKnownFolderPath != NULL )
				{
					bool free_memory = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							free_memory = InitializeOle32();
						}
					#endif

					// Make sure we can call CoTaskMemFree.
					if ( free_memory )
					{
						wchar_t *folder_path;

						for ( unsigned char i = 0; i < 6; ++i )
						{
							folder_path = NULL;
							if ( _SHGetKnownFolderPath( ( REFKNOWNFOLDERID )( *( guids[ i ] ) ), 0, ( HANDLE )NULL, &folder_path ) == S_OK )
							{
								folder_paths[ i ] = GlobalStrDupW( folder_path );
							}

							if ( folder_path != NULL )
							{
								_CoTaskMemFree( folder_path );
							}
						}
					}
				}
			}

			// Legacy folders.
			/*short csidl[ 6 ] = { CSIDL_DESKTOPDIRECTORY,
								 CSIDL_MYDOCUMENTS,
								 CSIDL_MYDOCUMENTS,	// My Documents\Downloads
								 CSIDL_MYMUSIC,
								 CSIDL_MYPICTURES,
								 CSIDL_MYVIDEO };*/
			short csidl[ 6 ];
			csidl[ 0 ] = CSIDL_DESKTOPDIRECTORY;
			csidl[ 1 ] = CSIDL_MYDOCUMENTS;
			csidl[ 2 ] = CSIDL_MYDOCUMENTS;	// My Documents\Downloads
			csidl[ 3 ] = CSIDL_MYMUSIC;
			csidl[ 4 ] = CSIDL_MYPICTURES;
			csidl[ 5 ] = CSIDL_MYVIDEO;

			/*wchar_t *default_categories[ 6 ] = { ST_V_Desktop,
												 ST_V_Documents,
												 ST_V_Downloads,
												 ST_V_Music,
												 ST_V_Pictures,
												 ST_V_Videos };*/
			wchar_t *default_categories[ 6 ];
			default_categories[ 0 ] = ST_V_Desktop;
			default_categories[ 1 ] = ST_V_Documents;
			default_categories[ 2 ] = ST_V_Downloads;
			default_categories[ 3 ] = ST_V_Music;
			default_categories[ 4 ] = ST_V_Pictures;
			default_categories[ 5 ] = ST_V_Videos;

			unsigned int directory_length;

			for ( unsigned char i = 0; i < 6; ++i )
			{
				if ( folder_paths[ i ] == NULL )
				{
					folder_paths[ i ] = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
					_SHGetFolderPathW( NULL, csidl[ i ], NULL, 0, folder_paths[ i ] );

					if ( i == 2 )		// Downloads
					{
						directory_length = lstrlenW( folder_paths[ i ] );
						folder_paths[ i ][ directory_length ] = L'\\';
						_wmemcpy_s( folder_paths[ i ] + ( directory_length + 1 ), MAX_PATH - ( directory_length + 1 ), ST_V_Downloads, ST_L_Downloads + 1 );
						folder_paths[ i ][ MAX_PATH - 1 ] = 0;	// Sanity.

						// Check to see if the new path exists and create it if it doesn't.
						if ( GetFileAttributesW( folder_paths[ i ] ) == INVALID_FILE_ATTRIBUTES )
						{
							CreateDirectoryW( folder_paths[ i ], NULL );
						}
					}
				}

				CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_INFO_ ) );
				if ( ci != NULL )
				{
					ci->category = GlobalStrDupW( default_categories[ i ] );
					ci->file_extensions = NULL;
					ci->download_directory = folder_paths[ i ];

					if ( dllrbt_insert( g_category_info, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
					{
						FreeCategoryInfo( &ci );
					}
					else
					{
						CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
						cti->data = ( void * )ci;
						cti->type = CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO;
						DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )cti );

						DLL_AddNode( &g_category_list, dll_node, -1 );

						CacheCategory( ci->category );
					}
				}
				else
				{
					GlobalFree( folder_paths[ i ] );
				}
			}

			category_list_changed = true;
		}

		ret_status = -1;	// Can't open file for reading.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading categories: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_category_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\categories\0", 12 );
	//g_base_directory[ g_base_directory_length + 11 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving categories: %S", g_base_directory );
#endif

	HANDLE hFile = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_CATEGORIES, sizeof( char ) * 4 );	// Magic identifier for the category info.
		pos += ( sizeof( char ) * 4 );

		DoublyLinkedList *dll_node = g_category_list;
		while ( dll_node != NULL )
		{
			CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
			if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
			{
				CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;

				// lstrlen is safe for NULL values.
				int category_length = ( lstrlenW( ci->category ) + 1 ) * sizeof( wchar_t );
				int file_extensions_length = ( lstrlenW( ci->file_extensions ) + 1 ) * sizeof( wchar_t );
				int download_directory_length = ( lstrlenW( ci->download_directory ) + 1 ) * sizeof( wchar_t );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( category_length + file_extensions_length + download_directory_length ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
#ifdef ENABLE_LOGGING
					lfz += write;
#endif
				}

				_memcpy_s( buf + pos, size - pos, ci->category, category_length );
				pos += category_length;

				_memcpy_s( buf + pos, size - pos, ci->file_extensions, file_extensions_length );
				pos += file_extensions_length;

				_memcpy_s( buf + pos, size - pos, ci->download_directory, download_directory_length );
				pos += download_directory_length;
			}

			dll_node = dll_node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
#ifdef ENABLE_LOGGING
			lfz += write;
#endif
		}

		GlobalFree( buf );

		SetEndOfFile( hFile );

		UnlockFileEx( hFile, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving categories: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

THREAD_RETURN load_category_list( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	TVINSERTSTRUCT tvis;
	_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

	tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
	tvis.item.stateMask = TVIS_EXPANDED;

	DoublyLinkedList *dll_node = g_category_list;
	while ( dll_node != NULL )
	{
		CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
		if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
		{
			CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;

			tvis.item.state = TVIS_EXPANDED;
			tvis.item.pszText = ci->category;
			tvis.item.lParam = ( LPARAM )dll_node;

			tvis.hParent = g_hti_categories;
			tvis.hInsertAfter = TVI_LAST;

			_SendMessageW( g_hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
		}

		dll_node = dll_node->next;
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN load_window_category_list( void *pArguments )
{
	HWND hWnd_cb = ( HWND )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( hWnd_cb != NULL )
	{
		bool *update_category_window = NULL;

		HWND hWnd = _GetParent( hWnd_cb );
		if ( hWnd == g_hWnd_add_urls )
		{
			update_category_window = &g_update_add_category_window;
		}
		else if ( hWnd == g_hWnd_update_download )
		{
			g_update_update_category_window = true;
			update_category_window = &g_update_update_category_window;
		}
		else if ( hWnd == g_hWnd_site_manager )
		{
			g_update_site_manager_category_window = true;
			update_category_window = &g_update_site_manager_category_window;
		}

		if ( update_category_window != NULL && *update_category_window )
		{
			*update_category_window = false;

			_SendMessageW( hWnd_cb, CB_RESETCONTENT, 0, 0 );
			_SendMessageW( hWnd_cb, CB_ADDSTRING, 0, ( LPARAM )ST_V_Default );

			DoublyLinkedList *dll_node = g_category_list;
			while ( dll_node != NULL )
			{
				CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
				if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
				{
					CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;

					int added_index = ( int )_SendMessageW( hWnd_cb, CB_ADDSTRING, 0, ( LPARAM )ci->category );
					_SendMessageW( hWnd_cb, CB_SETITEMDATA, added_index, ( LPARAM )ci );
				}

				dll_node = dll_node->next;
			}
		}

		_SendMessageW( hWnd, WM_UPDATE_CATEGORY, 0, NULL );
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

void RefreshSelectedFilter( unsigned int status, wchar_t *category )
{
	ProcessingList( true );

	LONG total_item_count = 0;
	LONG expanded_item_count = 0;
	LONG root_item_count = 0;

	TLV_SetStatusFilter( STATUS_NONE );
	TLV_SetCategoryFilter( NULL );

	TLV_ClearSelected( false, false );

	if ( status != UINT_MAX )
	{
		TLV_SetStatusFilter( status );
	}
	TLV_SetCategoryFilter( category );

	TREELISTNODE *node = g_tree_list;
	while ( node != NULL )
	{
		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )node->data;
		if ( status == STATUS_NONE || IsFilterSet( di, status ) )
		{
			total_item_count += ( node->child_count + 1 );
			++root_item_count;
			++expanded_item_count;	// Include the parent.
			if ( node->is_expanded )
			{
				expanded_item_count += node->child_count;
			}
		}

		node = node->next;
	}

	TLV_SetTotalItemCount( total_item_count );
	TLV_SetExpandedItemCount( expanded_item_count );
	TLV_SetRootItemCount( root_item_count );

	TLV_ResetSelectionBounds();
	TLV_SetFirstVisibleItem( g_tree_list );
	TLV_SetFirstVisibleIndex( 0 );
	TLV_SetFirstVisibleRootIndex( 0 );
	TLV_SetSelectedCount( 0 );

	TLV_SetFocusedItem( NULL );
	TLV_SetFocusedIndex( -1 );

	UpdateSBItemCount();

	ProcessingList( false );
}

// Return: 0 = Nothing done, 1 = Can't add extension(s), 2 = Extension(s) added/updated/removed
unsigned char AddUpdateRemoveCategoryFileExtensions( CATEGORY_INFO_ *old_ci, CATEGORY_INFO_ *new_ci )
{
	if ( g_category_file_extensions != NULL )
	{
		wchar_t *file_extension_start;
		wchar_t *file_extension_end;

		if ( new_ci != NULL && new_ci->file_extensions != NULL )
		{
			file_extension_start = new_ci->file_extensions;
			file_extension_end = new_ci->file_extensions;

			// See if the new file extensions aleady exist in the same category, or they don't exist in any category.
			// If not, then bail with a return value of 1.
			for ( ;; )
			{
				if ( *file_extension_end == 0 || *file_extension_end == L';' || *file_extension_end == L',' )
				{
					wchar_t tmp_end = *file_extension_end;
					*file_extension_end = NULL;

					// Find the category file extension info.
					CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )dllrbt_find( g_category_file_extensions, ( void * )file_extension_start, true );

					*file_extension_end = tmp_end;	// Restore.

					if ( cfei != NULL && cfei->ci != old_ci )
					{
						return 1;	// Extension exists in another category.

						break;
					}

					if ( tmp_end == 0 )
					{
						break;
					}

					file_extension_start = file_extension_end + 1;
				}

				++file_extension_end;
			}
		}

		if ( old_ci != NULL && old_ci->file_extensions != NULL )
		{
			file_extension_start = old_ci->file_extensions;
			file_extension_end = old_ci->file_extensions;

			// The new file extensions can be added to the category.
			// Remove all of the old file extensions from the category.
			for ( ;; )
			{
				if ( *file_extension_end == 0 || *file_extension_end == L';' || *file_extension_end == L',' )
				{
					wchar_t tmp_end = *file_extension_end;
					*file_extension_end = NULL;

					// Find the category file extension info.
					dllrbt_iterator *itr = dllrbt_find( g_category_file_extensions, ( void * )file_extension_start, false );

					*file_extension_end = tmp_end;	// Restore.

					if ( itr != NULL )
					{
						CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )( ( node_type * )itr )->val;
						if ( cfei != NULL )
						{
							GlobalFree( cfei->file_extension );
							GlobalFree( cfei );
						}

						dllrbt_remove( g_category_file_extensions, itr );
					}

					if ( tmp_end == 0 )
					{
						break;
					}

					file_extension_start = file_extension_end + 1;
				}

				++file_extension_end;
			}
		}

		if ( new_ci != NULL && new_ci->file_extensions != NULL )
		{
			file_extension_start = new_ci->file_extensions;
			file_extension_end = new_ci->file_extensions;

			// Add the new file extensions to the category.
			for ( ;; )
			{
				if ( *file_extension_end == 0 || *file_extension_end == L';' || *file_extension_end == L',' )
				{
					wchar_t tmp_end = *file_extension_end;
					*file_extension_end = NULL;

					int file_extension_length = ( int )( file_extension_end - file_extension_start );
					if ( file_extension_length > 0 )
					{
						CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_FILE_EXTENSION_INFO ) );
						cfei->ci = old_ci;

						cfei->file_extension = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( file_extension_length + 1 ) );
						_wmemcpy_s( cfei->file_extension, file_extension_length + 1, file_extension_start, file_extension_length );
						cfei->file_extension[ file_extension_length ] = 0;	// Sanity.

						if ( dllrbt_insert( g_category_file_extensions, ( void * )cfei->file_extension, ( void * )cfei ) != DLLRBT_STATUS_OK )
						{
							// Already exits.
							GlobalFree( cfei->file_extension );
							GlobalFree( cfei );
						}
					}

					if ( tmp_end == 0 )
					{
						break;
					}

					file_extension_start = file_extension_end + 1;
					*file_extension_end = tmp_end;	// Restore.
				}

				++file_extension_end;
			}
		}

		return 2;	// Added/Updated/Removed extensions.
	}

	return 0;
}

THREAD_RETURN handle_category_list( void *pArguments )
{
	CATEGORY_UPDATE_INFO *cui = ( CATEGORY_UPDATE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	CATEGORY_INFO_ *category_list_ci = NULL;
	unsigned char update_type = 0;

	if ( cui != NULL )
	{
		update_type = cui->update_type;

		unsigned char alert = 0;	// 1 = Category already exits, 2 = Extension exists in another category.

		if ( cui->update_type == 0 && cui->ci != NULL )	// Add
		{
#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Adding %S to categories", cui->ci->category );
#endif

			CATEGORY_INFO_ *ci = cui->ci;

			if ( dllrbt_insert( g_category_info, ( void * )ci, ( void * )ci ) != DLLRBT_STATUS_OK )
			{
				// Already exits.
				FreeCategoryInfo( &ci );

				alert = 1;	// Category already exists.
			}
			else
			{
				unsigned char added_extensions = AddUpdateRemoveCategoryFileExtensions( NULL, ci );
				if ( added_extensions == 2 )	// All extensions were added.
				{
					CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_TREE_INFO ) );
					cti->data = ( void * )ci;
					cti->type = CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO;
					DoublyLinkedList *dll_node = DLL_CreateNode( ( void * )cti );

					if ( g_category_list == NULL )
					{
						g_category_list = dll_node;
					}

					DLL_AddNode( &g_treeview_list, dll_node, -1 );

					CacheCategory( ci->category );

					TVINSERTSTRUCT tvis;
					_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

					tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
					tvis.item.stateMask = TVIS_EXPANDED;
					tvis.item.state = TVIS_EXPANDED;
					tvis.item.pszText = ci->category;
					tvis.item.lParam = ( LPARAM )dll_node;

					tvis.hParent = g_hti_categories;
					tvis.hInsertAfter = TVI_LAST;

					_SendMessageW( g_hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

					category_list_ci = ci;

					category_list_changed = true;
				}
				else if ( added_extensions == 1 )	// Already exists in another category.
				{
					// Find the category info
					dllrbt_iterator *itr = dllrbt_find( g_category_info, ( void * )ci, false );
					if ( itr != NULL )
					{
						dllrbt_remove( g_category_info, itr );
					}

					// Already exits.
					FreeCategoryInfo( &ci );

					alert = 2;	// Extension exists in another category.
				}
				else
				{
					// Find the category info
					dllrbt_iterator *itr = dllrbt_find( g_category_info, ( void * )ci, false );
					if ( itr != NULL )
					{
						dllrbt_remove( g_category_info, itr );
					}

					// Something went wrong when adding to the extension tree.
					FreeCategoryInfo( &ci );
				}
			}
		}
		else if ( cui->update_type == 1 && cui->ci != NULL && cui->old_ci != NULL && cui->hti != NULL )	// Update
		{
#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Updating %S to %S in categories", cui->old_ci->category, cui->ci->category );
#endif

			CATEGORY_INFO_ *ci = cui->ci;
			CATEGORY_INFO_ *old_ci = cui->old_ci;

			// Find the category info
			dllrbt_iterator *new_itr = dllrbt_find( g_category_info, ( void * )ci, false );

			// Find the category info
			dllrbt_iterator *old_itr = dllrbt_find( g_category_info, ( void * )cui->old_ci, false );

			if ( new_itr == NULL || old_itr == new_itr )
			{
				unsigned char added_extensions = AddUpdateRemoveCategoryFileExtensions( old_ci, ci );
				if ( added_extensions == 2 )	// All extensions were added.
				{
					wchar_t *t_file_extensions = old_ci->file_extensions;
					old_ci->file_extensions = ci->file_extensions;
					GlobalFree( t_file_extensions );

					RemoveCachedCategory( old_ci->category );
					wchar_t *category = CacheCategory( ci->category );

					dllrbt_remove( g_category_info, old_itr );

					wchar_t *t_category = old_ci->category;
					wchar_t *t_download_directory = old_ci->download_directory;
					old_ci->category = ci->category;
					old_ci->download_directory = ci->download_directory;

					GlobalFree( t_category );
					GlobalFree( t_download_directory );
					GlobalFree( ci );

					// Re-add it with the new information. It should not exist.
					dllrbt_insert( g_category_info, ( void * )old_ci, ( void * )old_ci );

					TVITEM tvi;
					_memzero( &tvi, sizeof( TVITEM ) );
					tvi.mask = TVIF_TEXT;
					tvi.hItem = cui->hti;
					tvi.pszText = cui->old_ci->category;
					_SendMessageW( g_hWnd_categories, TVM_SETITEM, 0, ( LPARAM )&tvi );

					RefreshSelectedFilter( CATEGORY_STATUS, category );

					category_list_ci = cui->old_ci;

					category_list_changed = true;
				}
				else if ( added_extensions == 1 )	// Already exists in another category.
				{
					// Already exits.
					FreeCategoryInfo( &ci );

					alert = 2;	// Extension exists in another category.
				}
				else
				{
					// Something went wrong when adding to the extension tree.
					FreeCategoryInfo( &ci );
				}
			}
			else
			{
				// Already exits.
				FreeCategoryInfo( &ci );

				alert = 1;	// Category already exists.
			}
		}
		else if ( cui->update_type == 2 && cui->hti != NULL )	// Remove
		{
			TVITEM tvi;
			_memzero( &tvi, sizeof( TVITEM ) );
			tvi.mask = TVIF_PARAM;
			tvi.hItem = cui->hti;
			_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvi );

			DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
			if ( dll_node != NULL )
			{
				CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
				if ( cti != NULL )
				{
					CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;
					if ( ci != NULL )
					{
						AddUpdateRemoveCategoryFileExtensions( ci, NULL );

						RemoveCachedCategory( ci->category );

						category_list_ci = ci;	// This pointer value (and only the value) will be used to find the item to remove in the category lists.
#ifdef ENABLE_LOGGING
						WriteLog( LOG_INFO_ACTION, "Removing %S from categories", ci->category );
#endif
						// Find the category info
						dllrbt_iterator *itr = dllrbt_find( g_category_info, ( void * )ci, false );
						if ( itr != NULL )
						{
							dllrbt_remove( g_category_info, itr );
						}

						FreeCategoryInfo( &ci );
					}

					GlobalFree( cti );
				}

				if ( dll_node == g_category_list )
				{
					g_category_list = g_category_list->next;
				}

				DLL_RemoveNode( &g_treeview_list, dll_node );
				GlobalFree( dll_node );
			}

			_SendMessageW( g_hWnd_categories, TVM_DELETEITEM, 0, ( LPARAM )cui->hti );

			RefreshSelectedFilter( STATUS_NONE, NULL );

			category_list_changed = true;
		}

		GlobalFree( cui );

		if ( alert == 1 )
		{
			_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_The_specified_category_already_exists );
		}
		else if ( alert == 2 )
		{
			_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_file_extensions_exist );
		}
	}

	if ( category_list_ci != NULL )
	{
		g_update_add_category_window = true;
		g_update_update_category_window = true;
		g_update_site_manager_category_window = true;

		HWND hWnd;
		HWND hWnd_cb;
		bool *update_category_window;
		int sel_index = 0;
		int update_index = 0;
		bool got_update_index = false;

		for ( char i = 0; i < 3; ++i )
		{
			hWnd = NULL;
			hWnd_cb = NULL;
			update_category_window = NULL;

			switch ( i )
			{
				case 0:	// Add URL(s) window.
				{
					if ( g_hWnd_add_urls != NULL )
					{
						hWnd = g_hWnd_add_urls;
						hWnd_cb = g_hWnd_au_category;
						update_category_window = &g_update_add_category_window;
					}
				}
				break;

				case 1:	// Update Download window.
				{
					if ( g_hWnd_update_download != NULL )
					{
						hWnd = g_hWnd_update_download;
						hWnd_cb = g_hWnd_update_category;
						update_category_window = &g_update_update_category_window;
					}
				}
				break;

				case 2:	// Site Manager Window
				{
					if ( g_hWnd_site_manager != NULL )
					{
						hWnd = g_hWnd_site_manager;
						hWnd_cb = g_hWnd_sm_category;
						update_category_window = &g_update_site_manager_category_window;
					}
				}
				break;
			}

			if ( update_category_window != NULL && *update_category_window &&
				 hWnd != NULL && _IsWindowVisible( hWnd ) == TRUE )
			{
				*update_category_window = false;

				if ( category_list_ci != NULL )
				{
					sel_index = ( int )_SendMessageW( hWnd_cb, CB_GETCURSEL, 0, 0 );

					if ( update_type == 0 )	// Add
					{
						int added_index = ( int )_SendMessageW( hWnd_cb, CB_ADDSTRING, 0, ( LPARAM )category_list_ci->category );
						_SendMessageW( hWnd_cb, CB_SETITEMDATA, added_index, ( LPARAM )category_list_ci );
					}
					else
					{
						if ( !got_update_index )
						{
							got_update_index = true;

							int count = ( int )_SendMessageW( hWnd_cb, CB_GETCOUNT, 0, 0 );
							for ( int j = 1; j < count; ++j )
							{
								LRESULT ret = _SendMessageW( hWnd_cb, CB_GETITEMDATA, j, 0 );
								if ( ret != CB_ERR )
								{
									if ( category_list_ci == ( CATEGORY_INFO_ * )ret )
									{
										update_index = j;

										break;
									}
								}
							}
						}

						if ( update_index > 0 )
						{
							_SendMessageW( hWnd_cb, CB_DELETESTRING, update_index, 0 );

							if ( update_type == 1 )	// Update
							{
								_SendMessageW( hWnd_cb, CB_INSERTSTRING, update_index, ( LPARAM )category_list_ci->category );
								_SendMessageW( hWnd_cb, CB_SETITEMDATA, update_index, ( LPARAM )category_list_ci );
							}
							else if ( update_type == 2 )	// Remove
							{
								if ( sel_index == update_index )
								{
									sel_index = 0;
								}
								else if ( sel_index > update_index )
								{
									--sel_index;
								}
							}
						}
					}
				}

				_SendMessageW( hWnd_cb, CB_SETCURSEL, sel_index, 0 );
				_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( _GetDlgCtrlID( hWnd_cb ), CBN_SELCHANGE ), 0 );
			}
		}
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_category_move( void *pArguments )
{
	CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( cti != NULL )
	{
		wchar_t *category = ( cti->data != NULL ? ( ( CATEGORY_INFO_ * )cti->data )->category : NULL );

		wchar_t *category_filter = TLV_GetCategoryFilter();

		TREELISTNODE *tln;
		TLV_GetNextSelectedItem( NULL, 0, &tln );

		while ( tln != NULL )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			// Ignore children.
			if ( tln->parent != NULL )
			{
				// Set tln to the last child so that TLV_GetNextSelectedItem can get the next selected.
				if ( tln->parent->child != NULL )
				{
					tln = ( tln->parent->child->prev != NULL ? tln->parent->child->prev : tln->parent->child );
				}
			}
			else
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					wchar_t *old_category = di->category;
					di->category = CacheCategory( category );
					old_category = RemoveCachedCategory( old_category );

					if ( di->tln != NULL )
					{
						TREELISTNODE *tln = ( TREELISTNODE * )di->tln;
						int child_count = ( tln->is_expanded ? tln->child_count : 0 );

						if ( category_filter != NULL )
						{
							// The download has been added to the currently filtered category.
							if ( di->category == category_filter )
							{
								TLV_AddTotalItemCount( 1 );
								TLV_AddExpandedItemCount( child_count + 1 );
								TLV_AddRootItemCount( 1 );
							}
							else if ( old_category == category_filter )	// The download has been removed from the currently filtered category.
							{
								TLV_AddTotalItemCount( -1 );
								TLV_AddExpandedItemCount( -( child_count + 1 ) );
								TLV_AddRootItemCount( -1 );
							}
						}
					}
				}
			}

			TLV_GetNextSelectedItem( tln, 0, &tln );
		}

		g_download_history_changed = true;

		UpdateSBItemCount();
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

wchar_t *CacheCategory( wchar_t *category )
{
	SHARED_CATEGORY_INFO *sci = NULL;

	if ( category != NULL )
	{
		sci = ( SHARED_CATEGORY_INFO * )dllrbt_find( g_shared_categories, ( void * )category, true );
		if ( sci == NULL )
		{
			sci = ( SHARED_CATEGORY_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SHARED_CATEGORY_INFO ) );
			if ( sci != NULL )
			{
				sci->category = GlobalStrDupW( category );
				sci->count = 1;

				if ( dllrbt_insert( g_shared_categories, ( void * )sci->category, ( void * )sci ) != DLLRBT_STATUS_OK )
				{
					GlobalFree( sci->category );
					GlobalFree( sci );
					sci = NULL;
				}
			}
		}
		else
		{
			++( sci->count );
		}
	}

	return ( sci != NULL ? sci->category : NULL );
}

wchar_t *RemoveCachedCategory( wchar_t *category )
{
	SHARED_CATEGORY_INFO *sci = NULL;

	if ( category != NULL )
	{
		dllrbt_iterator *itr = dllrbt_find( g_shared_categories, ( void * )category, false );

		// Free its values and remove it from the tree if there are no other items using it.
		if ( itr != NULL )
		{
			sci = ( SHARED_CATEGORY_INFO * )( ( node_type * )itr )->val;
			if ( sci != NULL )
			{
				if ( --sci->count == 0 )
				{
					GlobalFree( sci->category );
					GlobalFree( sci );
					sci = NULL;

					dllrbt_remove( g_shared_categories, itr );
				}
			}
			else
			{
				dllrbt_remove( g_shared_categories, itr );
			}
		}
	}

	return ( sci != NULL ? sci->category : NULL );
}
