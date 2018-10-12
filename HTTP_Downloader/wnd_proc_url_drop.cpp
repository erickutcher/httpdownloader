/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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
#include "lite_ole32.h"
#include "drag_and_drop.h"
#include "menus.h"
#include "resource.h"

HWND g_hWnd_url_drop_window = NULL;

struct WINDOW_SETTINGS
{
	TRACKMOUSEEVENT tme;
	POINT window_position;
	POINT drag_position;
	bool is_dragging;
	bool is_tracking;
};

HBITMAP hbm_background = NULL;

WINDOW_SETTINGS window_settings;

bool use_drag_and_drop_url = true;	// Assumes OLE32_STATE_RUNNING is true.
IDropTarget *URL_DropTarget;

LRESULT CALLBACK URLDropWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			window_settings.window_position.x = 0;
			window_settings.window_position.y = 0;
			window_settings.drag_position.x = 0;
			window_settings.drag_position.y = 0;
			window_settings.is_dragging = false;

			window_settings.tme.cbSize = sizeof( TRACKMOUSEEVENT );
			window_settings.tme.dwFlags = TME_LEAVE;
			window_settings.tme.dwHoverTime = HOVER_DEFAULT;
			window_settings.tme.hwndTrack = hWnd;

			if ( hbm_background == NULL )
			{
				// Need to delete the object when destroying this window.
				hbm_background = ( HBITMAP )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP_DROP ), IMAGE_BITMAP, 48, 48, 0 );
			}

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop_url = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop_url )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( hWnd, &URL_DropTarget );
			}

			return 0;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm_background );
			_DeleteObject( ohbm );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, 0, 0, 48, 48, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_CAPTURECHANGED:
		{
			window_settings.is_dragging = ( ( HWND )lParam == hWnd ) ? true : false;

			return 0;
		}
		break;

		case WM_LBUTTONDOWN:
		{
			_SetCapture( hWnd );

			_GetCursorPos( &window_settings.drag_position );
			RECT rc;
			_GetWindowRect( hWnd, &rc );
			window_settings.window_position.x = rc.left;
			window_settings.window_position.y = rc.top;

			window_settings.is_dragging = true;

			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			_ReleaseCapture();

			return 0;
		}
	    break;

		case WM_MOUSEMOVE:
		{
			// See if we have the mouse captured.
			if ( window_settings.is_dragging )
			{
				POINT cur_pos;
				RECT wa;
				RECT rc;
				_GetWindowRect( hWnd, &rc );

				_GetCursorPos( &cur_pos );
				_OffsetRect( &rc, cur_pos.x - ( rc.left + window_settings.drag_position.x - window_settings.window_position.x ), cur_pos.y - ( rc.top + window_settings.drag_position.y - window_settings.window_position.y ) );

				// Allow our main window to attach to the desktop edge.
				_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
				if( is_close( rc.left, wa.left ) )				// Attach to left side of the desktop.
				{
					_OffsetRect( &rc, wa.left - rc.left, 0 );
				}
				else if ( is_close( wa.right, rc.right ) )		// Attach to right side of the desktop.
				{
					_OffsetRect( &rc, wa.right - rc.right, 0 );
				}

				if( is_close( rc.top, wa.top ) )				// Attach to top of the desktop.
				{
					_OffsetRect( &rc, 0, wa.top - rc.top );
				}
				else if ( is_close( wa.bottom, rc.bottom ) )	// Attach to bottom of the desktop.
				{
					_OffsetRect( &rc, 0, wa.bottom - rc.bottom );
				}

				_SetWindowPos( hWnd, NULL, rc.left, rc.top, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE );

				// Save our settings for the position/dimensions of the window.
				cfg_drop_pos_x = rc.left;
				cfg_drop_pos_y = rc.top;
			}

			if ( !window_settings.is_tracking )
			{
				window_settings.is_tracking = true;

				TrackMouseEvent( &window_settings.tme );

				_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, 0xFF, LWA_ALPHA );
			}

			return 0;
		}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			// This will capture MoveWindow and SetWindowPos changes.
			WINDOWPOS *wp = ( WINDOWPOS * )lParam;

			if ( !( wp->flags & SWP_NOMOVE ) )
			{
				cfg_drop_pos_x = wp->x;
				cfg_drop_pos_y = wp->y;
			}

			// Let it fall through so we can still get the WM_SIZE message.
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_MOUSELEAVE:
		{
			if ( window_settings.is_tracking )
			{
				window_settings.is_tracking = false;

				_SetLayeredWindowAttributes( hWnd, 0, 0x80, LWA_ALPHA );
			}

			return 0;
		}
		break;

		case WM_NCHITTEST:
		{
			// Allow us to work in the client area, but nothing else (resize, title bar, etc.).
			return ( _DefWindowProcW( hWnd, msg, wParam, lParam ) == HTCLIENT ? HTCLIENT : NULL );
		}
		break;

		case WM_RBUTTONUP:
		{
			// Show our tray context menu as a popup.
			POINT p;
			_GetCursorPos( &p ) ;
			_TrackPopupMenu( g_hMenuSub_tray, 0, p.x, p.y, 0, g_hWnd_main, NULL );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			_DeleteObject( hbm_background );
			hbm_background = NULL;

			g_hWnd_url_drop_window = NULL;

			if ( use_drag_and_drop_url )
			{
				UnregisterDropWindow( hWnd, URL_DropTarget );

				_OleUninitialize();
			}

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
