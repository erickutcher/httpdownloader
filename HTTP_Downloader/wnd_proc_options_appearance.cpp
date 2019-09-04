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

#include "options.h"
#include "lite_gdi32.h"
#include "lite_ole32.h"
#include "lite_comdlg32.h"
#include "utilities.h"

#define LB_ROW_OPTIONS				1000

#define LB_PROGRESS_COLOR			1001
#define LB_PROGRESS_COLOR_OPTIONS	1002

#define BTN_SHOW_GRID_LINES			1003

#define BTN_SORT_ADDED_AND_UPDATING_ITEMS	1004

// Appearance Tab
HWND g_hWnd_row_options_list = NULL;
HWND g_hWnd_static_example_row = NULL;
HWND g_hWnd_progress_color_list = NULL;
HWND g_hWnd_static_example_progress = NULL;

HWND g_hWnd_progress_color_options_list = NULL;

HWND g_hWnd_chk_show_gridlines = NULL;

HWND g_hWnd_chk_sort_added_and_updating_items = NULL;

COLORREF t_odd_row_background_color;
COLORREF t_even_row_background_color;

COLORREF t_odd_row_highlight_color;
COLORREF t_even_row_highlight_color;

COLORREF t_odd_row_highlight_font_color;
COLORREF t_even_row_highlight_font_color;

COLORREF t_progress_colors[ NUM_COLORS ];

FONT_SETTINGS t_odd_row_font_settings;
FONT_SETTINGS t_even_row_font_settings;

void SetAppearance()
{
	for ( unsigned char i = 0; i < NUM_COLORS; ++i )
	{
		t_progress_colors[ i ] = *progress_colors[ i ];
	}

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
			g_hWnd_row_options_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 15, 200, 85, hWnd, ( HMENU )LB_ROW_OPTIONS, NULL, NULL );
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

			g_hWnd_static_example_row = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 205, 15, 74, 45, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_show_gridlines = _CreateWindowW( WC_BUTTON, ST_V_Show_gridlines_in_download_list, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 85, rc.right - 205, 20, hWnd, ( HMENU )BTN_SHOW_GRID_LINES, NULL, NULL );


			HWND hWnd_static_progress_color = _CreateWindowW( WC_STATIC, ST_V_Progress_bar_, WS_CHILD | WS_VISIBLE, 0, 105, rc.right, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_progress_color_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 120, 200, 85, hWnd, ( HMENU )LB_PROGRESS_COLOR, NULL, NULL );
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

			g_hWnd_progress_color_options_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTBOX, NULL, LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 205, 120, 140, 85, hWnd, ( HMENU )LB_PROGRESS_COLOR_OPTIONS, NULL, NULL );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Progress_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Background_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Progress_Font_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Background_Font_Color );
			_SendMessageW( g_hWnd_progress_color_options_list, LB_ADDSTRING, 0, ( LPARAM )ST_V_Border_Color );

			g_hWnd_static_example_progress = _CreateWindowW( WC_STATIC, NULL, SS_OWNERDRAW | WS_BORDER | WS_CHILD | WS_VISIBLE, 350, 120, rc.right - 350, 45, hWnd, NULL, NULL, NULL );


			g_hWnd_chk_sort_added_and_updating_items = _CreateWindowW( WC_BUTTON, ST_V_Sort_added_and_updating_items, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 210, rc.right, 20, hWnd, ( HMENU )BTN_SORT_ADDED_AND_UPDATING_ITEMS, NULL, NULL );


			_SendMessageW( hWnd_static_row_options, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_row_options_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_example_row, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_static_progress_color, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_progress_color_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_progress_color_options_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_example_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_gridlines, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_sort_added_and_updating_items, WM_SETFONT, ( WPARAM )g_hFont, 0 );


			_SendMessageW( g_hWnd_chk_show_gridlines, BM_SETCHECK, ( cfg_show_gridlines ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_sort_added_and_updating_items, BM_SETCHECK, ( cfg_sort_added_and_updating_items ? BST_CHECKED : BST_UNCHECKED ), 0 );

			SetAppearance();

			_SendMessageW( g_hWnd_row_options_list, LB_SETCURSEL, 0, 0 );
			_SendMessageW( g_hWnd_progress_color_list, LB_SETCURSEL, 0, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
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

				case LB_ROW_OPTIONS:
				{
					if ( HIWORD( wParam ) == LBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_row_options_list, LB_GETCURSEL, 0, 0 );
						if ( index != LB_ERR )
						{
							_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );

							if ( index != 0 && index != 5 )	// Odd/Even Row/Font/Highlight Color
							{
								CHOOSECOLOR cc;
								_memzero( &cc, sizeof( CHOOSECOLOR ) );
								static COLORREF CustomColors[ 16 ];
								_memzero( &CustomColors, sizeof( CustomColors ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = CustomColors;
								cc.hwndOwner = hWnd;

								switch ( index )
								{
									case 1: { cc.rgbResult = t_odd_row_background_color; } break;
									case 2: { cc.rgbResult = t_odd_row_font_settings.font_color; } break;
									case 3: { cc.rgbResult = t_odd_row_highlight_color; } break;
									case 4: { cc.rgbResult = t_odd_row_highlight_font_color; } break;

									case 6: { cc.rgbResult = t_even_row_background_color; } break;
									case 7: { cc.rgbResult = t_even_row_font_settings.font_color; } break;
									case 8: { cc.rgbResult = t_even_row_highlight_color; } break;
									case 9: { cc.rgbResult = t_even_row_highlight_font_color; } break;
								}

								if ( _ChooseColorW( &cc ) == TRUE )
								{
									switch ( index )
									{
										case 1: { t_odd_row_background_color = cc.rgbResult; } break;
										case 2: { t_odd_row_font_settings.font_color = cc.rgbResult; } break;
										case 3: { t_odd_row_highlight_color = cc.rgbResult; } break;
										case 4: { t_odd_row_highlight_font_color = cc.rgbResult; } break;

										case 6: { t_even_row_background_color = cc.rgbResult; } break;
										case 7: { t_even_row_font_settings.font_color = cc.rgbResult; } break;
										case 8: { t_even_row_highlight_color = cc.rgbResult; } break;
										case 9: { t_even_row_highlight_font_color = cc.rgbResult; } break;
									}

									_InvalidateRect( g_hWnd_static_example_row, NULL, TRUE );

									options_state_changed = true;
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
							else// if ( index == 0 || index == 5 )	// Odd/Even Row Font
							{
								FONT_SETTINGS *fs;

								if ( index == 0 )
								{
									fs = &t_odd_row_font_settings;
								}
								else //if ( index == 5 )
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
									_EnableWindow( g_hWnd_apply, TRUE );
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
								static COLORREF CustomColors[ 16 ];
								_memzero( &CustomColors, sizeof( CustomColors ) );

								cc.lStructSize = sizeof( CHOOSECOLOR );
								cc.Flags = CC_FULLOPEN | CC_RGBINIT;
								cc.lpCustColors = CustomColors;
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
									_EnableWindow( g_hWnd_apply, TRUE );
								}
							}
						}
					}
				}
				break;

				case BTN_SHOW_GRID_LINES:
				case BTN_SORT_ADDED_AND_UPDATING_ITEMS:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
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
						FONT_SETTINGS *fs;
						COLORREF background_color;
						COLORREF font_color;
						HFONT row_font;

						if ( index >= 0 && index <= 4 )
						{
							fs = &t_odd_row_font_settings;
						}
						else //if ( index >= 5 && index <= 9 )
						{
							fs = &t_even_row_font_settings;
						}

						row_font = fs->font;

						if ( index == 3 || index == 4 )
						{
							font_color = t_odd_row_highlight_font_color;
							background_color = t_odd_row_highlight_color;
						}
						else if ( index == 8 || index == 9 )
						{
							font_color = t_even_row_highlight_font_color;
							background_color = t_even_row_highlight_color;
						}
						else
						{
							font_color = fs->font_color;

							if ( index >= 0 && index <= 4 )
							{
								background_color = t_odd_row_background_color;
							}
							else //if ( index >= 5 && index <= 9 )
							{
								background_color = t_even_row_background_color;
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

						HBRUSH color = _CreateSolidBrush( background_color );
						_FillRect( hdcMem, &dis->rcItem, color );
						_DeleteObject( color );

						_SetTextColor( hdcMem, font_color );
						_DrawTextW( hdcMem, buf, -1, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

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
						HBRUSH color = _CreateSolidBrush( t_progress_colors[ index + 1 ]/*( COLORREF )_GetSysColor( COLOR_WINDOW )*/ );
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
	return TRUE;
}
