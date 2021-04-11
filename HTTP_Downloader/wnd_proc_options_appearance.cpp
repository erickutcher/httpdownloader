/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2021 Eric Kutcher

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
#include "lite_ole32.h"
#include "lite_comdlg32.h"
#include "utilities.h"

#define LB_ROW_OPTIONS						1000

#define LB_PROGRESS_COLOR					1001
#define LB_PROGRESS_COLOR_OPTIONS			1002

#define LB_TD_PROGRESS_COLOR				1003
#define LB_TD_PROGRESS_COLOR_OPTIONS		1004

#define BTN_DRAW_FULL_ROWS					1005
#define BTN_DRAW_ALL_ROWS					1006
#define BTN_SHOW_GRIDLINES					1007

#define BTN_SHOW_PART_PROGRESS				1008

#define BTN_SORT_ADDED_AND_UPDATING_ITEMS	1009
#define BTN_EXPAND_ADDED_GROUP_ITEMS		1010

// Appearance Tab
HWND g_hWnd_row_options_list = NULL;
HWND g_hWnd_static_example_row = NULL;
HWND g_hWnd_progress_color_list = NULL;
HWND g_hWnd_static_example_progress = NULL;

HWND g_hWnd_progress_color_options_list = NULL;

HWND g_hWnd_td_progress_color_list = NULL;
HWND g_hWnd_td_progress_color_options_list = NULL;
HWND g_hWnd_static_example_td_progress = NULL;

HWND g_hWnd_chk_show_gridlines = NULL;
HWND g_hWnd_chk_draw_full_rows = NULL;
HWND g_hWnd_chk_draw_all_rows = NULL;
HWND g_hWnd_chk_show_part_progress = NULL;

HWND g_hWnd_chk_sort_added_and_updating_items = NULL;
HWND g_hWnd_chk_expand_added_group_items = NULL;

bool t_show_gridlines;
bool t_draw_full_rows;
bool t_draw_all_rows;

COLORREF t_background_color;
COLORREF t_gridline_color;

COLORREF t_selection_marquee_color;

COLORREF t_odd_row_background_color;
COLORREF t_even_row_background_color;

COLORREF t_odd_row_highlight_color;
COLORREF t_even_row_highlight_color;

COLORREF t_odd_row_highlight_font_color;
COLORREF t_even_row_highlight_font_color;

COLORREF t_progress_colors[ NUM_COLORS ];

COLORREF t_td_progress_colors[ TD_NUM_COLORS ];

FONT_SETTINGS t_odd_row_font_settings;
FONT_SETTINGS t_even_row_font_settings;

FONT_SETTINGS *last_row_fs;
COLORREF last_row_background_color;
COLORREF last_row_font_color;
COLORREF opposite_row_background_color;

void SetAppearance()
{
	for ( unsigned char i = 0; i < NUM_COLORS; ++i )
	{
		t_progress_colors[ i ] = *progress_colors[ i ];
	}

	for ( unsigned char i = 0; i < TD_NUM_COLORS; ++i )
	{
		t_td_progress_colors[ i ] = *td_progress_colors[ i ];
	}

	t_show_gridlines = cfg_show_gridlines;
	t_draw_full_rows = cfg_draw_full_rows;
	t_draw_all_rows = cfg_draw_all_rows;

	t_background_color = cfg_background_color;
	t_gridline_color = cfg_gridline_color;
	t_selection_marquee_color = cfg_selection_marquee_color;

	t_odd_row_background_color = cfg_odd_row_background_color;
	t_even_row_background_color = cfg_even_row_background_color;

	t_odd_row_highlight_color = cfg_odd_row_highlight_color;
	t_even_row_highlight_color = cfg_even_row_highlight_color;

	t_odd_row_highlight_font_color = cfg_odd_row_highlight_font_color;
	t_even_row_highlight_font_color = cfg_even_row_highlight_font_color;

	t_odd_row_font_settings.font = _CreateFontIndirectW( &cfg_odd_row_font_settings.lf );
	t_odd_row_font_settings.font_color = cfg_odd_row_font_settings.font_color;
	//t_odd_row_font_settings.lf = cfg_odd_row_font_settings.lf;
	_memcpy_s( &t_odd_row_font_settings.lf, sizeof( LOGFONT ), &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ) );

	t_even_row_font_settings.font = _CreateFontIndirectW( &cfg_even_row_font_settings.lf );
	t_even_row_font_settings.font_color = cfg_even_row_font_settings.font_color;
	//t_even_row_font_settings.lf = cfg_even_row_font_settings.lf;
	_memcpy_s( &t_even_row_font_settings.lf, sizeof( LOGFONT ), &cfg_even_row_font_settings.lf, sizeof( LOGFONT ) );
}

LRESULT CALLBACK AppearanceTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_row_options = _CreateWindowW( WC_STATIC, ST_V_Download_list_, WS_CHILD | WS_VISIBLE, 0, 0, rc.right, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_row_options_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 15, 250, 95, hWnd, ( HMENU )LB_ROW_OPTIONS, NULL, NULL );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Background_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Gridline_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Selection_Marquee_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Odd_Row_Font );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Odd_Row_Background_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Odd_Row_Font_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Odd_Row_Highlight_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Odd_Row_Highlight_Font_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Even_Row_Font );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Even_Row_Background_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Even_Row_Font_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Even_Row_Highlight_Color );
			_SendMessageW( g_hWnd_row_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Even_Row_Highlight_Font_Color );

			g_hWnd_static_example_row = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 255, 15, 180, 95, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_show_gridlines = _CreateWindowW( WC_BUTTON, ST_V_Show_gridlines_in_download_list, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 115, rc.right, 20, hWnd, ( HMENU )BTN_SHOW_GRIDLINES, NULL, NULL );
			g_hWnd_chk_draw_full_rows = _CreateWindowW( WC_BUTTON, ST_V_Draw_full_rows, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 135, rc.right, 20, hWnd, ( HMENU )BTN_DRAW_FULL_ROWS, NULL, NULL );
			g_hWnd_chk_draw_all_rows = _CreateWindowW( WC_BUTTON, ST_V_Draw_all_rows, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 155, rc.right, 20, hWnd, ( HMENU )BTN_DRAW_ALL_ROWS, NULL, NULL );

			/*HWND hWnd_static_hoz1 =*/ _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 185, rc.right, 1, hWnd, NULL, NULL, NULL );

			HWND hWnd_static_progress_color = _CreateWindowW( WC_STATIC, ST_V_Progress_bar_, WS_CHILD | WS_VISIBLE, 0, 195, rc.right, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_progress_color_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 210, 250, 95, hWnd, ( HMENU )LB_PROGRESS_COLOR, NULL, NULL );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Allocating_File );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Authorization_Required );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Completed );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Connecting );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Downloading );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Failed );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_File_IO_Error );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Moving_File );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Paused );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Proxy_Authentication_Required );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Queued );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Restarting );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Skipped );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Stopped );
			_SendMessageW( g_hWnd_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Timed_Out );

			g_hWnd_progress_color_options_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 310, 250, 95, hWnd, ( HMENU )LB_PROGRESS_COLOR_OPTIONS, NULL, NULL );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Progress_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Background_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Progress_Font_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Background_Font_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Border_Color );

			g_hWnd_static_example_progress = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 255, 210, 180, 50, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_show_part_progress = _CreateWindowW( WC_BUTTON, ST_V_Show_progress_for_each_part, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 410, rc.right, 20, hWnd, ( HMENU )BTN_SHOW_PART_PROGRESS, NULL, NULL );

			/*HWND hWnd_static_proxy_hoz2 =*/ _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 440, rc.right, 1, hWnd, NULL, NULL, NULL );

			HWND hWnd_static_td_progress_color = _CreateWindowW( WC_STATIC, ST_V_Other_progress_bars_, WS_CHILD | WS_VISIBLE, 0, 450, rc.right, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_td_progress_color_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 465, 250, 80, hWnd, ( HMENU )LB_TD_PROGRESS_COLOR, NULL, NULL );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_System_Tray_Icon_Downloading );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_System_Tray_Icon_Paused );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_System_Tray_Icon_Error );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_URL_Drop_Window_Downloading );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_URL_Drop_Window_Paused );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_URL_Drop_Window_Error );

			g_hWnd_td_progress_color_options_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 550, 160, 60, hWnd, ( HMENU )LB_TD_PROGRESS_COLOR_OPTIONS, NULL, NULL );
			_SendMessageW( g_hWnd_td_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Progress_Color );
			_SendMessageW( g_hWnd_td_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Border_Color );

			g_hWnd_static_example_td_progress = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 255, 465, 180, 50, hWnd, NULL, NULL, NULL );

			/*HWND hWnd_static_proxy_hoz3 =*/ _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 625, rc.right, 1, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_sort_added_and_updating_items = _CreateWindowW( WC_BUTTON, ST_V_Sort_added_and_updating_items, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 635, rc.right, 20, hWnd, ( HMENU )BTN_SORT_ADDED_AND_UPDATING_ITEMS, NULL, NULL );
			g_hWnd_chk_expand_added_group_items = _CreateWindowW( WC_BUTTON, ST_V_Expand_added_group_items, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 655, rc.right, 20, hWnd, ( HMENU )BTN_EXPAND_ADDED_GROUP_ITEMS, NULL, NULL );


			SCROLLINFO si;
			_memzero( &si, sizeof( SCROLLINFO ) );
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = 675;	// Bottom of the last item in the window.
			si.nPage = rc.bottom - rc.top;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );


			_SendMessageW( hWnd_static_row_options, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_row_options_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_example_row, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_progress_color, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_progress_color_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_progress_color_options_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_example_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_td_progress_color, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_td_progress_color_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_td_progress_color_options_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_example_td_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_gridlines, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_draw_full_rows, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_draw_all_rows, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_part_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_sort_added_and_updating_items, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_expand_added_group_items, WM_SETFONT, ( WPARAM )g_hFont, 0 );


			_SendMessageW( g_hWnd_chk_show_gridlines, BM_SETCHECK, ( cfg_show_gridlines ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_draw_full_rows, BM_SETCHECK, ( cfg_draw_full_rows ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_draw_all_rows, BM_SETCHECK, ( cfg_draw_all_rows ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_show_part_progress, BM_SETCHECK, ( cfg_show_part_progress ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_sort_added_and_updating_items, BM_SETCHECK, ( cfg_sort_added_and_updating_items ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_expand_added_group_items, BM_SETCHECK, ( cfg_expand_added_group_items ? BST_CHECKED : BST_UNCHECKED ), 0 );

			SetAppearance();

			_SendMessageW( g_hWnd_row_options_list, LB_SETCURSEL, 0, 0 );
			_SendMessageW( g_hWnd_progress_color_list, LB_SETCURSEL, 0, 0 );
			_SendMessageW( g_hWnd_td_progress_color_list, LB_SETCURSEL, 0, 0 );

			return 0;
		}
		break;

		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			_SetFocus( hWnd );

			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			int delta = si.nPos;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { si.nPos -= 10; } break;
					case SB_LINEDOWN: { si.nPos += 10; } break;
					case SB_PAGEUP: { si.nPos -= si.nPage; } break;
					case SB_PAGEDOWN: { si.nPos += si.nPage; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { si.nPos = ( int )HIWORD( wParam ); } break;
					case SB_TOP: { si.nPos = 0; } break;
					case SB_BOTTOM: { si.nPos = ( si.nMax - si.nPage ) + 1; } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				si.nPos -= ( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * 20;
			}

			_SetScrollPos( hWnd, SB_VERT, si.nPos, TRUE );

			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			if ( si.nPos != delta )
			{
				_ScrollWindow( hWnd, 0, delta - si.nPos, NULL, NULL );
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case LB_PROGRESS_COLOR:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						_InvalidateRect( g_hWnd_static_example_progress, NULL, TRUE );
					}
				}
				break;

				case LB_TD_PROGRESS_COLOR:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						_InvalidateRect( g_hWnd_static_example_td_progress, NULL, TRUE );
					}
				}
				break;

				case LB_ROW_OPTIONS:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_row_options_list, LB_GETCURSEL, 0, 0 );
						if ( index != LB_ERR )
						{
							_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );

							if ( index != 3 && index != 8 )	// Background/Gridline/Odd/Even Row/Font/Highlight Color
							{
								CHOOSECOLOR cc;
								_memzero( &cc, sizeof( CHOOSECOLOR ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = g_CustColors;
								cc.hwndOwner = hWnd;

								switch ( index )
								{
									case 0: { cc.rgbResult = t_background_color; } break;
									case 1: { cc.rgbResult = t_gridline_color; } break;
									case 2: { cc.rgbResult = t_selection_marquee_color; } break;

									case 4: { cc.rgbResult = t_odd_row_background_color; } break;
									case 5: { cc.rgbResult = t_odd_row_font_settings.font_color; } break;
									case 6: { cc.rgbResult = t_odd_row_highlight_color; } break;
									case 7: { cc.rgbResult = t_odd_row_highlight_font_color; } break;

									case 9: { cc.rgbResult = t_even_row_background_color; } break;
									case 10: { cc.rgbResult = t_even_row_font_settings.font_color; } break;
									case 11: { cc.rgbResult = t_even_row_highlight_color; } break;
									case 12: { cc.rgbResult = t_even_row_highlight_font_color; } break;
								}

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									switch ( index )
									{
										case 0: { t_background_color = cc.rgbResult; } break;
										case 1: { t_gridline_color = cc.rgbResult; } break;
										case 2: { t_selection_marquee_color = cc.rgbResult; } break;

										case 4: { t_odd_row_background_color = cc.rgbResult; } break;
										case 5: { t_odd_row_font_settings.font_color = cc.rgbResult; } break;
										case 6: { t_odd_row_highlight_color = cc.rgbResult; } break;
										case 7: { t_odd_row_highlight_font_color = cc.rgbResult; } break;

										case 9: { t_even_row_background_color = cc.rgbResult; } break;
										case 10: { t_even_row_font_settings.font_color = cc.rgbResult; } break;
										case 11: { t_even_row_highlight_color = cc.rgbResult; } break;
										case 12: { t_even_row_highlight_font_color = cc.rgbResult; } break;
									}

									_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_options_apply, TRUE );
								}
							}
							else// if ( index == 3 || index == 8 )	// Odd/Even Row Font
							{
								FONT_SETTINGS *fs;

								if ( index == 3 )
								{
									fs = &t_odd_row_font_settings;
								}
								else //if ( index == 8 )
								{
									fs = &t_even_row_font_settings;
								}

								CHOOSEFONT cf;
								_memzero( &cf, sizeof( CHOOSEFONT ) );
								cf.lStructSize = sizeof( CHOOSEFONT );
								cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
								cf.lpLogFont = &fs->lf;
								cf.hwndOwner = hWnd;
								cf.rgbColors = fs->font_color;

								if ( _ChooseFontW( &cf ) == TRUE )
								{
									fs->font_color = cf.rgbColors;

									if ( fs->font != NULL )
									{
										_DeleteObject( fs->font );
									}
									fs->font = _CreateFontIndirectW( cf.lpLogFont );

									_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_options_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case LB_PROGRESS_COLOR_OPTIONS:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_progress_color_list, LB_GETCURSEL, 0, 0 );
						if ( index != LB_ERR )
						{
							int option_index = ( int )_SendMessageW( g_hWnd_progress_color_options_list, LB_GETCURSEL, 0, 0 );
							if ( option_index != LB_ERR )
							{
								index *= 5;

								CHOOSECOLOR cc;
								_memzero( &cc, sizeof( CHOOSECOLOR ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = g_CustColors;
								cc.hwndOwner = hWnd;

								switch ( option_index )
								{
									case 0: { cc.rgbResult = t_progress_colors[ index ]; } break;		// Progress Color
									case 1: { cc.rgbResult = t_progress_colors[ index + 1 ]; } break;	// Background Color
									case 2: { cc.rgbResult = t_progress_colors[ index + 2 ]; } break;	// Progress Text Color
									case 3: { cc.rgbResult = t_progress_colors[ index + 3 ]; } break;	// Background Text Color
									case 4: { cc.rgbResult = t_progress_colors[ index + 4 ]; } break;	// Border Color
								}

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									switch ( option_index )
									{
										case 0: { t_progress_colors[ index ] = cc.rgbResult; } break;		// Progress Color
										case 1: { t_progress_colors[ index + 1 ] = cc.rgbResult; } break;	// Background Color
										case 2: { t_progress_colors[ index + 2 ] = cc.rgbResult; } break;	// Progress Text Color
										case 3: { t_progress_colors[ index + 3 ] = cc.rgbResult; } break;	// Background Text Color
										case 4: { t_progress_colors[ index + 4 ] = cc.rgbResult; } break;	// Border Color
									}

									_InvalidateRect( g_hWnd_static_example_progress, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_options_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case LB_TD_PROGRESS_COLOR_OPTIONS:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_td_progress_color_list, LB_GETCURSEL, 0, 0 );
						if ( index != LB_ERR )
						{
							int option_index = ( int )_SendMessageW( g_hWnd_td_progress_color_options_list, LB_GETCURSEL, 0, 0 );
							if ( option_index != LB_ERR )
							{
								index *= 2;

								CHOOSECOLOR cc;
								_memzero( &cc, sizeof( CHOOSECOLOR ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = g_CustColors;
								cc.hwndOwner = hWnd;

								switch ( option_index )
								{
									case 0: { cc.rgbResult = t_td_progress_colors[ index ]; } break;		// Progress Color
									case 1: { cc.rgbResult = t_td_progress_colors[ index + 1 ]; } break;	// Border Color
								}

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									switch ( option_index )
									{
										case 0: { t_td_progress_colors[ index ] = cc.rgbResult; } break;		// Progress Color
										case 1: { t_td_progress_colors[ index + 1 ] = cc.rgbResult; } break;	// Border Color
									}

									_InvalidateRect( g_hWnd_static_example_td_progress, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_options_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case BTN_SHOW_GRIDLINES:
				{
					t_show_gridlines = ( _SendMessageW( g_hWnd_chk_show_gridlines, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );

					_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );
				}
				break;

				case BTN_DRAW_FULL_ROWS:
				{
					t_draw_full_rows = ( _SendMessageW( g_hWnd_chk_draw_full_rows, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );

					_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );
				}
				break;

				case BTN_DRAW_ALL_ROWS:
				{
					t_draw_all_rows = ( _SendMessageW( g_hWnd_chk_draw_all_rows, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );

					_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );
				}
				break;

				case BTN_SHOW_PART_PROGRESS:
				case BTN_SORT_ADDED_AND_UPDATING_ITEMS:
				case BTN_EXPAND_ADDED_GROUP_ITEMS:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our static control.
			if ( dis->CtlType == ODT_STATIC )
			{
				int index = LB_ERR;
				wchar_t *buf = L"AaBbYyZz";

				if ( dis->hwndItem == g_hWnd_static_example_row )
				{
					index = ( int )_SendMessageW( g_hWnd_row_options_list, LB_GETCURSEL, 0, 0 );
					if ( index != LB_ERR )
					{
						HFONT row_font;

						bool set_last_row_fs = ( last_row_fs == NULL ? true : false );

						if ( index >= 3 && index <= 7 || set_last_row_fs )
						{
							last_row_fs = &t_odd_row_font_settings;
						}
						else if ( index >= 8 && index <= 12 )
						{
							last_row_fs = &t_even_row_font_settings;
						}

						row_font = last_row_fs->font;

						if ( index == 6 || index == 7 )
						{
							last_row_font_color = t_odd_row_highlight_font_color;
							last_row_background_color = t_odd_row_highlight_color;

							opposite_row_background_color = t_even_row_background_color;
						}
						else if ( index == 11 || index == 12 )
						{
							last_row_font_color = t_even_row_highlight_font_color;
							last_row_background_color = t_even_row_highlight_color;

							opposite_row_background_color = t_odd_row_background_color;
						}
						else
						{
							last_row_font_color = last_row_fs->font_color;

							if ( index >= 3 && index <= 7 || set_last_row_fs )
							{
								last_row_background_color = t_odd_row_background_color;

								opposite_row_background_color = t_even_row_background_color;
							}
							else if ( index >= 8 && index <= 12 )
							{
								last_row_background_color = t_even_row_background_color;

								opposite_row_background_color = t_odd_row_background_color;
							}
						}

						int width = dis->rcItem.right - dis->rcItem.left;
						int height = dis->rcItem.bottom - dis->rcItem.top;

						HDC hdcMem = _CreateCompatibleDC( dis->hDC );
						HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
						HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
						_DeleteObject( ohbm );
						_DeleteObject( hbm );
						HFONT ohf = ( HFONT )_SelectObject( hdcMem, row_font );
						_DeleteObject( ohf );

						// Transparent background for text.
						_SetBkMode( hdcMem, TRANSPARENT );

						HBRUSH background = _CreateSolidBrush( t_background_color );
						_FillRect( hdcMem, &dis->rcItem, background );
						_DeleteObject( background );

						RECT row_rc;
						row_rc.top = 0;
						row_rc.bottom = 50;
						row_rc.left = 0;
						row_rc.right = ( t_draw_full_rows ? width : 130 );

						background = _CreateSolidBrush( last_row_background_color );
						_FillRect( hdcMem, &row_rc, background );
						_DeleteObject( background );

						if ( t_draw_all_rows )
						{
							row_rc.top = 51;
							row_rc.bottom = height;

							background = _CreateSolidBrush( opposite_row_background_color );
							_FillRect( hdcMem, &row_rc, background );
							_DeleteObject( background );

							row_rc.top = 0;
							row_rc.bottom = 50;
						}

						row_rc.right = 130;

						_SetTextColor( hdcMem, last_row_font_color );
						_DrawTextW( hdcMem, buf, -1, &row_rc, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						if ( t_show_gridlines )
						{
							HPEN line_color = _CreatePen( PS_SOLID, 4, t_gridline_color );
							HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
							_DeleteObject( old_color );

							_MoveToEx( hdcMem, width, 0, NULL );
							_LineTo( hdcMem, 0, 0 );
							_LineTo( hdcMem, 0, height );

							_MoveToEx( hdcMem, 0, row_rc.bottom, NULL );
							_LineTo( hdcMem, width, row_rc.bottom );

							_MoveToEx( hdcMem, row_rc.right, 0, NULL );
							_LineTo( hdcMem, row_rc.right, height );
						}

						/////////////////////////

						// Marquee.
						RECT drag_rc;
						drag_rc.top = 35;
						drag_rc.left = 50;
						drag_rc.bottom = drag_rc.top + 40;
						drag_rc.right = drag_rc.left + 100;

						HDC hdcMem2 = _CreateCompatibleDC( dis->hDC );
						HBITMAP hbm2 = _CreateCompatibleBitmap( dis->hDC, 1, 1 );
						HBITMAP ohbm2 = ( HBITMAP )_SelectObject( hdcMem2, hbm2 );
						_DeleteObject( ohbm2 );
						_DeleteObject( hbm2 );

						RECT body_rc;
						body_rc.left = 0;
						body_rc.top = 0;
						body_rc.right = 1;
						body_rc.bottom = 1;
						background = _CreateSolidBrush( t_selection_marquee_color );
						_FillRect( hdcMem2, &body_rc, background );

						// Blend the rectangle into the background to make it look transparent.
						//BLENDFUNCTION blend = { AC_SRC_OVER, 0, 85, AC_SRC_OVER };	// 85 matches Explorer's Detail view.
						//BLENDFUNCTION blend = { AC_SRC_OVER, 0, 70, AC_SRC_OVER };	// 70 matches a ListView control.
						BLENDFUNCTION blend;
						blend.BlendOp = AC_SRC_OVER;
						blend.BlendFlags = 0;
						blend.SourceConstantAlpha = 70;
						blend.AlphaFormat = AC_SRC_OVER;

						_GdiAlphaBlend( hdcMem, drag_rc.left, drag_rc.top, drag_rc.right - drag_rc.left, drag_rc.bottom - drag_rc.top, hdcMem2, 0, 0, 1, 1, blend );
						_DeleteDC( hdcMem2 );

						// Draw a solid border around rectangle.
						_FrameRect( hdcMem, &drag_rc, background );

						_DeleteObject( background );

						/////////////////////////

						_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height, hdcMem, 0, 0, SRCCOPY );

						// Delete our back buffer.
						_DeleteDC( hdcMem );
					}
				}
				else if ( dis->hwndItem == g_hWnd_static_example_progress )
				{
					index = ( int )_SendMessageW( g_hWnd_progress_color_list, LB_GETCURSEL, 0, 0 );
					if ( index != LB_ERR )
					{
						index *= 5;

						int width = dis->rcItem.right - dis->rcItem.left;
						int height = dis->rcItem.bottom - dis->rcItem.top;

						RECT rc_clip = dis->rcItem;
						rc_clip.right /= 2;

						HDC hdcMem = _CreateCompatibleDC( dis->hDC );
						HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
						HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
						_DeleteObject( ohbm );
						_DeleteObject( hbm );
						HFONT ohf = ( HFONT )_SelectObject( hdcMem, g_hFont );
						_DeleteObject( ohf );

						// Transparent background for text.
						_SetBkMode( hdcMem, TRANSPARENT );

						// Background Color
						HBRUSH color = _CreateSolidBrush( t_progress_colors[ index + 1 ] );
						_FillRect( hdcMem, &dis->rcItem, color );
						_DeleteObject( color );

						// Background Font Color
						_SetTextColor( hdcMem, t_progress_colors[ index + 3 ] );
						_DrawTextW( hdcMem, buf, -1, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height, hdcMem, 0, 0, SRCCOPY );

						// Progress Color
						color = _CreateSolidBrush( t_progress_colors[ index ] );
						_FillRect( hdcMem, &rc_clip, color );
						_DeleteObject( color );

						// Progress Font Color
						_SetTextColor( hdcMem, t_progress_colors[ index + 2 ] );
						_DrawTextW( hdcMem, buf, -1, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						_BitBlt( dis->hDC, dis->rcItem.left, dis->rcItem.top, ( rc_clip.right - rc_clip.left ), height, hdcMem, 0, 0, SRCCOPY );

						// Border Color
						HPEN hPen = _CreatePen( PS_SOLID, 4, t_progress_colors[ index + 4 ] );
						_SelectObject( dis->hDC, hPen );
						_SelectObject( dis->hDC, _GetStockObject( NULL_BRUSH ) );
						_Rectangle( dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height );
						_DeleteObject( hPen );

						// Delete our back buffer.
						_DeleteDC( hdcMem );
					}
				}
				else if ( dis->hwndItem == g_hWnd_static_example_td_progress )
				{
					index = ( int )_SendMessageW( g_hWnd_td_progress_color_list, LB_GETCURSEL, 0, 0 );
					if ( index != LB_ERR )
					{
						index *= 2;

						int width = dis->rcItem.right - dis->rcItem.left;
						int height = dis->rcItem.bottom - dis->rcItem.top;

						// Progress Color
						HBRUSH color = _CreateSolidBrush( t_td_progress_colors[ index ] );
						_FillRect( dis->hDC, &dis->rcItem, color );
						_DeleteObject( color );

						// Border Color
						HPEN hPen = _CreatePen( PS_SOLID, 4, t_td_progress_colors[ index + 1 ] );
						_SelectObject( dis->hDC, hPen );
						_SelectObject( dis->hDC, _GetStockObject( NULL_BRUSH ) );
						_Rectangle( dis->hDC, dis->rcItem.left, dis->rcItem.top, width, height );
						_DeleteObject( hPen );
					}
				}
			}
			return TRUE;
		}
		break;

		case WM_DESTROY:
		{
			if ( t_even_row_font_settings.font != NULL ){ _DeleteObject( t_even_row_font_settings.font ); }
			if ( t_odd_row_font_settings.font != NULL ){ _DeleteObject( t_odd_row_font_settings.font ); }

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
