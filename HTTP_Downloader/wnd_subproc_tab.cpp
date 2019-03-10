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

#include "globals.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"

WNDPROC TabProc = NULL;

HTHEME hTheme = NULL;
unsigned int init_count = 0;

// Subclassed tab control.
LRESULT CALLBACK TabSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_PROPAGATE:
		{
			bool use_theme = true;
			#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					use_theme = InitializeUXTheme();
				}
			#endif

			if ( use_theme )
			{
				if ( hTheme == NULL )
				{
					// Open our tab theme. Closed in WM_DESTROY of the tab subclass.
					hTheme = _OpenThemeData( hWnd, L"Tab" );
				}

				if ( hTheme != NULL )
				{
					++init_count;

					return TRUE;
				}
			}

			// Let's not subclass this control if we can't open the theme data.
			_SetWindowLongPtrW( hWnd, GWLP_WNDPROC, ( LONG_PTR )TabProc );

			return FALSE;
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background painting in WM_PAINT.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			int width = client_rc.right - client_rc.left;
			int height = client_rc.bottom - client_rc.top;

			// Get a handle to the device context.
			HDC hDC = _BeginPaint( hWnd, &ps );

			// Create and save a bitmap in memory to paint to. (Double buffer)
			HDC hdcMem = _CreateCompatibleDC( hDC );
			HBITMAP hbm = _CreateCompatibleBitmap( hDC, width, height );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );

			// Fill the background.
			HBRUSH hBrush = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &client_rc, hBrush );
			_DeleteObject( hBrush );

			// Get the tab item's height. (bottom)
			RECT rc_tab;
			_SendMessageW( hWnd, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			// Draw our tab border.
			RECT rc_pane;
			rc_pane.left = 0;
			rc_pane.top = rc_tab.bottom;
			rc_pane.right = client_rc.right;
			rc_pane.bottom = client_rc.bottom;

			if ( hTheme != NULL )
			{
				_DrawThemeBackground( hTheme, hdcMem, TABP_PANE, 0, &rc_pane, 0 );
			}

			// Set our font.
			HFONT hf = ( HFONT )_SelectObject( hdcMem, g_hFont );
			// Delete our old font.
			_DeleteObject( hf );

			wchar_t tab_text[ 64 ];
			TCITEM tci;
			_memzero( &tci, sizeof( TCITEM ) );
			tci.mask = TCIF_TEXT;
			tci.pszText = tab_text;
			tci.cchTextMax = 64;

			int index = ( int )_SendMessageW( hWnd, TCM_GETCURSEL, 0, 0 );	// Get the selected tab

			TCHITTESTINFO tcht;
			tcht.flags = TCHT_ONITEM;
			_GetCursorPos( &tcht.pt );
			_ScreenToClient( hWnd, &tcht.pt );
			int cur_over = ( int )_SendMessageW( hWnd, TCM_HITTEST, 0, ( LPARAM )&tcht );

			// Get the bounding rect for each tab item.
			int tab_count = ( int )_SendMessageW( hWnd, TCM_GETITEMCOUNT, 0, 0 );
			for ( int i = 0; i < tab_count; ++i )
			{
				// Exclude the selected tab. We draw it last so it can clip the non-selected tabs.
				if ( i != index )
				{
					_SendMessageW( hWnd, TCM_GETITEMRECT, i, ( LPARAM )&rc_tab );

					if ( rc_tab.left >= 0 )
					{
						// If the mouse is over the current selection, then set it to normal.
						if ( i == cur_over )
						{
							// This is handling two scenarios.
							// 1. The last visible tab is partially hidden (it's right is greater than the window right). That means the Up/Down control is visible.
							// 2. The last completely visible tab and the partially hidden tab are within the dimensions of the Up/Down control.
							// We want to repaint the area for both tabs to prevent tearing.
							if ( rc_tab.right + ( 2 * _GetSystemMetrics( SM_CXHTHUMB ) ) >= client_rc.right )
							{
								_InvalidateRect( hWnd, &rc_tab, TRUE );
							}

							if ( hTheme != NULL )
							{
								_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_HOT, &rc_tab, 0 );
							}
						}
						else
						{
							if ( hTheme != NULL )
							{
								_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_NORMAL, &rc_tab, 0 );
							}
						}

						// Offset the text position.
						++rc_tab.top;

						_SetBkMode( hdcMem, TRANSPARENT );
						_SetTextColor( hdcMem, _GetSysColor( COLOR_BTNTEXT ) );

						_SendMessageW( hWnd, TCM_GETITEM, i, ( LPARAM )&tci );	// Get the tab's information.
						_DrawTextW( hdcMem, tci.pszText, -1, &rc_tab, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
					}
				}
			}

			// Draw the selected tab on top of the others.
			_SendMessageW( hWnd, TCM_GETITEMRECT, index, ( LPARAM )&rc_tab );

			// Only show the tab if it's completely visible.
			if ( rc_tab.left > 0 )
			{
				// Enlarge the selected tab's area.
				RECT rc_selected;
				rc_selected.left = rc_tab.left - 2;
				rc_selected.top = rc_tab.top - 2;
				rc_selected.right = rc_tab.right + 2;
				rc_selected.bottom = rc_tab.bottom + 2;

				// Exclude the 1 pixel white bottom.
				RECT rc_clip;
				rc_clip.left = rc_selected.left;
				rc_clip.top  = rc_selected.top;
				rc_clip.right = rc_selected.right;
				rc_clip.bottom = rc_selected.bottom - 1;

				if ( hTheme != NULL )
				{
					_DrawThemeBackground( hTheme, hdcMem, TABP_TABITEM, TIS_SELECTED, &rc_selected, &rc_clip );
				}

				// Position of our text (offset up).
				RECT rc_text;
				rc_text.left = rc_tab.left;
				rc_text.top  = rc_tab.top - 1;
				rc_text.right = rc_tab.right;
				rc_text.bottom = rc_tab.bottom - 2;

				_SetBkMode( hdcMem, TRANSPARENT );
				_SetTextColor( hdcMem, _GetSysColor( COLOR_BTNTEXT ) );

				_SendMessageW( hWnd, TCM_GETITEM, index, ( LPARAM )&tci );	// Get the tab's information.
				_DrawTextW( hdcMem, tci.pszText, -1, &rc_text, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
			}

			// Draw our back buffer.
			_BitBlt( hDC, client_rc.left, client_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );

			// Delete our bitmap.
			_DeleteObject( hbm );

			// Delete our back buffer.
			_DeleteDC( hdcMem );

			// Release the device context.
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( init_count > 0 && --init_count == 0 )
			{
				// Close the theme.
				if ( hTheme != NULL )
				{
					_CloseThemeData( hTheme );
				}
			}
		}
		break;
	}

	// Everything that we don't handle gets passed back to the parent to process.
	return _CallWindowProcW( TabProc, hWnd, msg, wParam, lParam );
}
