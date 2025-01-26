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

#include "dark_mode.h"

#ifdef ENABLE_DARK_MODE

#include "globals.h"

#include "lite_dlls.h"
#include "lite_shell32.h"
#include "lite_user32.h"
#include "lite_comctl32.h"

#include "vssym32.h"	// Theme properties.

#define DARK_MODE_STATE_SHUTDOWN	0
#define DARK_MODE_STATE_RUNNING		1

unsigned char dark_mode_state = DARK_MODE_STATE_SHUTDOWN;

//

// user32.dll
pGetMenuItemInfoW						_GetMenuItemInfoW;
pGetMenuBarInfo							_GetMenuBarInfo;
pEnumChildWindows						_EnumChildWindows;
pEnumThreadWindows						_EnumThreadWindows;
pGetClassNameW							_GetClassNameW;
//pRedrawWindow							_RedrawWindow;
pSetPropW								_SetPropW;
pRemovePropW							_RemovePropW;
pGetPropW								_GetPropW;
// Undocumented
//pSetWindowCompositionAttribute		_SetWindowCompositionAttribute;

// comctl32.dll
pImageList_Draw							_ImageList_Draw;
pImageList_SetImageCount				_ImageList_SetImageCount;
//pImageList_GetIcon						_ImageList_GetIcon;
pSetWindowSubclass						_SetWindowSubclass;
pDefSubclassProc						_DefSubclassProc;
pRemoveWindowSubclass					_RemoveWindowSubclass;
//

// gdi32.dll
pExcludeClipRect						_ExcludeClipRect;
pSetBkColor								_SetBkColor;
pPolygon								_Polygon;
pExtCreatePen							_ExtCreatePen;
//

// ntdll.dll
// Undocumented
pRtlGetNtVersionNumbers					_RtlGetNtVersionNumbers;
//

// uxtheme.dll
pSetWindowTheme							_SetWindowTheme;
pDrawThemeTextEx						_DrawThemeTextEx;
//pGetThemeRect							_GetThemeRect;
pBufferedPaintInit						_BufferedPaintInit;
pBufferedPaintUnInit					_BufferedPaintUnInit;
pBufferedPaintRenderAnimation			_BufferedPaintRenderAnimation;
pBeginBufferedAnimation					_BeginBufferedAnimation;
pEndBufferedAnimation					_EndBufferedAnimation;
pBufferedPaintStopAllAnimations			_BufferedPaintStopAllAnimations;
// Undocumented
// 1809 17763
//pOpenNcThemeData						_OpenNcThemeData;
//pRefreshImmersiveColorPolicyState		_RefreshImmersiveColorPolicyState;
//pGetIsImmersiveColorUsingHighContrast	_GetIsImmersiveColorUsingHighContrast;
//pShouldAppsUseDarkMode				_ShouldAppsUseDarkMode;
pAllowDarkModeForWindow					_AllowDarkModeForWindow;
pAllowDarkModeForApp					_AllowDarkModeForApp;
//pFlushMenuThemes						_FlushMenuThemes;
//pIsDarkModeAllowedForWindow			_IsDarkModeAllowedForWindow;
// 1903 18362
//pShouldSystemUseDarkMode				_ShouldSystemUseDarkMode;
pSetPreferredAppMode					_SetPreferredAppMode;
//pIsDarkModeAllowedForApp				_IsDarkModeAllowedForApp;
//

// dwmapi.dll
pDwmSetWindowAttribute					_DwmSetWindowAttribute;
//

//

#define BUTTON_ANIMATION_DURATION		200
#define CBRB_ANIMATION_DURATION			100

//

#define GLYPH_NONE						0
#define GLYPH_CB_UNCHECKED_NORMAL		1
#define GLYPH_CB_CHECKED_NORMAL			2
#define GLYPH_CB_UNCHECKED_HOT			3
#define GLYPH_CB_CHECKED_HOT			4
#define GLYPH_CB_UNCHECKED_PRESSED		5
#define GLYPH_CB_CHECKED_PRESSED		6
#define GLYPH_CB_UNCHECKED_DISABLED		7
#define GLYPH_CB_CHECKED_DISABLED		8
#define GLYPH_RB_UNCHECKED_NORMAL		9
#define GLYPH_RB_CHECKED_NORMAL			10
#define GLYPH_RB_UNCHECKED_HOT			11
#define GLYPH_RB_CHECKED_HOT			12
#define GLYPH_RB_UNCHECKED_PRESSED		13
#define GLYPH_RB_CHECKED_PRESSED		14
#define GLYPH_RB_UNCHECKED_DISABLED		15
#define GLYPH_RB_CHECKED_DISABLED		16

COLORREF dm_color_window_text =					RGB( 0xFF, 0xFF, 0xFF );
COLORREF dm_color_window_background =			RGB( 0x20, 0x20, 0x20 );
COLORREF dm_color_window_border =				RGB( 0x6B, 0x6B, 0x6B );

COLORREF dm_color_edit_background =				RGB( 0x19, 0x19, 0x19 );
COLORREF dm_color_edit_border_hover =			RGB( 0xBB, 0xBB, 0xBB );
COLORREF dm_color_edit_border_enabled =			RGB( 0x9B, 0x9B, 0x9B );
COLORREF dm_color_edit_border_disabled =		RGB( 0x40, 0x40, 0x40 );

COLORREF dm_color_button_normal =				RGB( 0x33, 0x33, 0x33 );
COLORREF dm_color_button_hover =				RGB( 0x45, 0x45, 0x45 );
COLORREF dm_color_button_pressed =				RGB( 0x66, 0x66, 0x66 );
COLORREF dm_color_button_border_hover =			RGB( 0xBB, 0xBB, 0xBB );
COLORREF dm_color_button_border_enabled =		RGB( 0x9B, 0x9B, 0x9B );
COLORREF dm_color_button_border_disabled =		RGB( 0x40, 0x40, 0x40 );

COLORREF dm_color_ud_button_border_enabled =	RGB( 0x9B, 0x9B, 0x9B );
COLORREF dm_color_ud_button_border_disabled =	RGB( 0x40, 0x40, 0x40 );
COLORREF dm_color_arrow =						RGB( 0xC8, 0xC8, 0xC8 );

COLORREF dm_color_menu_hover =					RGB( 0x41, 0x41, 0x41 );
//COLORREF dm_color_menu_border =				RGB( 0x80, 0x80, 0x80 );

COLORREF dm_color_focus_rectangle				RGB( 0xCC, 0xCC, 0xCC );

COLORREF dm_color_list_highlight =				RGB( 0x62, 0x62, 0x62 );

COLORREF dm_color_listbox_border =				RGB( 0x82, 0x87, 0x90 );	// Same as treeview and listview.

COLORREF dm_color_listview_grid					RGB( 0x40, 0x40, 0x40 );

COLORREF dm_color_toolbar_separator				RGB( 0x63, 0x63, 0x63 );	// Same has header separators.

bool g_use_dark_mode = false;

DWORD CBS_DARK_MODE = 0;
DWORD LBS_DARK_MODE = 0;

DWORD g_dm_buildNumber = 0;

DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 0;	// 19 for builds before 20H1, 20 for builds after.

HTHEME g_hTheme_menu = NULL;
HBRUSH g_hBrush_window_background = NULL;
HBRUSH g_hBrush_edit_background = NULL;

HTHEME hTheme_status = NULL;

HMODULE hModule_dm_user32 = NULL;
HMODULE hModule_dm_comctl32 = NULL;
HMODULE hModule_dm_gdi32 = NULL;
HMODULE hModule_dm_ntdll = NULL;
HMODULE hModule_dm_uxtheme = NULL;
HMODULE hModule_dm_dwmapi = NULL;

wchar_t status_bar_tooltip_text[ 64 ];
HWND g_hWnd_status_bar_tooltip = NULL;
BOOL g_sb_is_tracking = FALSE;
int g_sb_tracking_x = -1;
int g_sb_tracking_y = -1;

wchar_t toolbar_tooltip_text[ 64 ];
HWND g_hWnd_toolbar_tooltip = NULL;
BOOL g_tb_is_tracking = FALSE;
int g_tb_tracking_x = -1;
int g_tb_tracking_y = -1;

//

struct SUBCLASS_INFO
{
	UINT_PTR id;
	DWORD ref;
};

SUBCLASS_INFO g_sci_list_box = { 0, 0 };
SUBCLASS_INFO g_sci_line = { 0, 0 };
SUBCLASS_INFO g_sci_static = { 0, 0 };
SUBCLASS_INFO g_sci_cbrb = { 0, 0 };
SUBCLASS_INFO g_sci_group_box = { 0, 0 };
SUBCLASS_INFO g_sci_up_down = { 0, 0 };
SUBCLASS_INFO g_sci_combo_box = { 0, 0 };
SUBCLASS_INFO g_sci_ip = { 0, 0 };
SUBCLASS_INFO g_sci_list_view = { 0, 0 };
SUBCLASS_INFO g_sci_tree_view = { 0, 0 };
SUBCLASS_INFO g_sci_status_bar = { 0, 0 };
SUBCLASS_INFO g_sci_control_color = { 0, 0 };
SUBCLASS_INFO g_sci_msg_box = { 0, 0 };
SUBCLASS_INFO g_sci_tab = { 0, 0 };
SUBCLASS_INFO g_sci_toolbar = { 0, 0 };
SUBCLASS_INFO g_sci_edit = { 0, 0 };
SUBCLASS_INFO g_sci_edit_child = { 0, 0 };

//

#define _SCALE_DM_( x )						_SCALE_( ( x ), dpi_dark_mode )

//

void CleanupButtonGlyphs( HWND hWnd )
{
	HANDLE glyphs = _GetPropW( hWnd, L"GLYPHS" );
	if ( glyphs != NULL )
	{
		unsigned char glyph_type = ( unsigned char )_GetPropW( hWnd, L"GLYPH_TYPE" );
		if ( glyph_type == 0 )
		{
			_ImageList_Destroy( ( HIMAGELIST )glyphs );
		}
		else if ( glyph_type == 1 )
		{
			_CloseThemeData( ( HTHEME )glyphs );
		}

		_RemovePropW( hWnd, L"GLYPH_HW" );
		_RemovePropW( hWnd, L"GLYPH_TYPE" );
		_RemovePropW( hWnd, L"GLYPHS" );
	}
}

HIMAGELIST UpdateButtonGlyphs( HWND hWnd, LONG *height_width )
{
	HBITMAP hBmp;

	_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\buttons.bmp\0", 13 );
	if ( GetFileAttributesW( g_program_directory ) != INVALID_FILE_ATTRIBUTES )
	{
		hBmp = ( HBITMAP )_LoadImageW( NULL, g_program_directory, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );
	}
	else
	{
		return NULL;
	}

	UINT current_dpi_dark_mode = ( UINT )_SendMessageW( _GetParent( hWnd ), WM_GET_DPI, 0, 0 );

	HDC hDC = _GetDC( hWnd );

	HDC hdcMem_bmp = _CreateCompatibleDC( hDC );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem_bmp, hBmp );
	_DeleteObject( ohbm );

	BITMAP bmp;
	_memzero( &bmp, sizeof( BITMAP ) );
	_GetObjectW( hBmp, sizeof( BITMAP ), &bmp );

	//int res_height = _SCALE_DM_( 13);
	//int res_width = _SCALE_DM_( 221 );
	int res_height = _SCALE_DM_( bmp.bmHeight );
	int res_width = res_height * 17;	// Ensures proportionality.

	if ( height_width != NULL )
	{
		*height_width = res_height;
	}

	HBITMAP hBmp_scaled = _CreateCompatibleBitmap( hDC, res_width, res_height );

	HDC hdcMem_scaled = _CreateCompatibleDC( hDC );
	ohbm = ( HBITMAP )_SelectObject( hdcMem_scaled, hBmp_scaled );
	_DeleteObject( ohbm );

	_SetStretchBltMode( hdcMem_scaled, COLORONCOLOR );
	_StretchBlt( hdcMem_scaled, 0, 0, res_width, res_height, hdcMem_bmp, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY );

	_DeleteDC( hdcMem_scaled );
	_DeleteDC( hdcMem_bmp );
	_ReleaseDC( hWnd, hDC );

	// bmBitsPixel should match the ILC_COLOR4, ILC_COLOR8, etc. masks.
	if ( bmp.bmBitsPixel < 4 || bmp.bmBitsPixel > 32 )
	{
		bmp.bmBitsPixel = 8;	// ILC_COLOR8
	}
	// Height and width of each icon is the same.
	HIMAGELIST hil = _ImageList_Create( res_height, res_height, ILC_MASK | bmp.bmBitsPixel, 17, 0 );
	_ImageList_AddMasked( hil, hBmp_scaled, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ) );

	_DeleteObject( hBmp_scaled );
	_DeleteObject( hBmp );

	return hil;
}

//

LRESULT CALLBACK DMListBoxSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_NCPAINT:
		{
			LRESULT ret = _DefSubclassProc( hWnd, msg, wParam, lParam );

			HDC hDC = _GetDC( hWnd );

			RECT client_rc;
			_GetWindowRect( hWnd, &client_rc );
			_MapWindowPoints( HWND_DESKTOP, hWnd, ( POINT * )&client_rc, 2 );

			HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_listbox_border );
			HPEN old_color = ( HPEN )_SelectObject( hDC, hPen );
			_DeleteObject( old_color );
			HBRUSH old_brush = ( HBRUSH )_SelectObject( hDC, _GetStockObject( NULL_BRUSH ) );
			_DeleteObject( old_brush );
			_Rectangle( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom );
			_DeleteObject( hPen );

			_ReleaseDC( hWnd, hDC );

			return ret;
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			HWND hWnd_parent = _GetParent( hWnd );

			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
			
			UINT last_dpi_dark_mode = ( UINT )_GetPropW( hWnd, L"DPI" );

			if ( current_dpi_dark_mode != 0 && last_dpi_dark_mode != 0 && current_dpi_dark_mode != last_dpi_dark_mode )
			{
				_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

				int height = ( int )_SendMessageW( hWnd, LB_GETITEMHEIGHT, 0, 0 );
				if ( height != LB_ERR )
				{
					_SendMessageW( hWnd, LB_SETITEMHEIGHT, 0, _SCALE2_( height, dpi_dark_mode ) );
				}
			}
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMListBoxSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMLineSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );
			client_rc.bottom = client_rc.top + 1;	// 1px line. Do not scale.

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			HBRUSH color = _CreateSolidBrush( dm_color_window_border );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMLineSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMStaticSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			HBRUSH color = _CreateSolidBrush( dm_color_window_background );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( ( style & 0x0000000F ) == SS_ICON )
			{
				HICON hIcon = ( HICON )_SendMessageW( hWnd, STM_GETICON, 0, 0 );
				_DrawIconEx( hdcMem, 0, 0, hIcon, 0, 0, NULL, NULL, DI_NORMAL );
			}
			else
			{
				HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
				HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
				if ( _IsWindowEnabled( hWnd ) == TRUE )
				{
					_SetTextColor( hdcMem, dm_color_window_text );
				}
				else
				{
					_SetTextColor( hdcMem, _GetSysColor( COLOR_GRAYTEXT ) );
				}
				// Transparent background for text.
				_SetBkMode( hdcMem, TRANSPARENT );

				unsigned int align;
				//DWORD style = ( DWORD )_GetWindowLongPtrW( hWnd, GWL_STYLE );
				if ( ( style & 0x0000000F ) == SS_RIGHT )
				{
					align = DT_RIGHT;
				}
				else if ( ( style & 0x0000000F ) == SS_CENTER )
				{
					align = DT_CENTER;
				}
				else
				{
					align = DT_LEFT;
				}

				wchar_t *buf;
				wchar_t tbuf[ 64 ];
				int len = ( int )_SendMessageW( hWnd, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL character.
				if ( len > 64 )
				{
					buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
				}
				else
				{
					buf = tbuf;
				}

				if ( buf != NULL )
				{
					len = ( int )_SendMessageW( hWnd, WM_GETTEXT, len, ( LPARAM )buf );

					_DrawTextW( hdcMem, buf, len, &client_rc, DT_NOPREFIX | align );

					if ( buf != tbuf )
					{
						GlobalFree( buf );
					}
				}

				_SelectObject( hdcMem, ohf );
			}

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMStaticSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void CBRBPaint( HWND hWnd, HDC hDC, COLORREF text_color, int draw_state )
{
	HWND hWnd_parent = _GetParent( hWnd );
	UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

	RECT client_rc;
	_GetClientRect( hWnd, &client_rc );

	int height = client_rc.bottom - client_rc.top;
	int width = client_rc.right - client_rc.left;

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	HBITMAP hbm = _CreateCompatibleBitmap( hDC, width, height );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
	_DeleteObject( ohbm );
	_DeleteObject( hbm );

	HBRUSH color = _CreateSolidBrush( dm_color_window_background );
	_FillRect( hdcMem, &client_rc, color );
	_DeleteObject( color );

	HANDLE glyphs = _GetPropW( hWnd_parent, L"GLYPHS" );
	if ( glyphs != NULL )
	{
		unsigned char glyph_type = ( unsigned char )_GetPropW( hWnd_parent, L"GLYPH_TYPE" );
		if ( glyph_type == 0 )
		{
			LONG height_width = ( LONG )_GetPropW( hWnd_parent, L"GLYPH_HW" );

			// Vertically center the button.
			_ImageList_Draw( ( HIMAGELIST )glyphs, draw_state, hdcMem, client_rc.left, client_rc.top + ( ( height - height_width ) / 2 ), 0 );
		}
		else if ( glyph_type == 1 )	// Default buttons if the buttons.bmp file is missing.
		{
			switch ( draw_state )
			{
				case GLYPH_CB_UNCHECKED_NORMAL:		{ draw_state = CBS_UNCHECKEDNORMAL; } break;
				case GLYPH_CB_CHECKED_NORMAL:		{ draw_state = CBS_CHECKEDNORMAL; } break;
				case GLYPH_CB_UNCHECKED_HOT:		{ draw_state = CBS_UNCHECKEDHOT; } break;
				case GLYPH_CB_CHECKED_HOT:			{ draw_state = CBS_CHECKEDHOT; } break;
				case GLYPH_CB_UNCHECKED_PRESSED:	{ draw_state = CBS_UNCHECKEDPRESSED; } break;
				case GLYPH_CB_CHECKED_PRESSED:		{ draw_state = CBS_CHECKEDPRESSED; } break;
				case GLYPH_CB_UNCHECKED_DISABLED:	{ draw_state = CBS_UNCHECKEDDISABLED; } break;
				case GLYPH_CB_CHECKED_DISABLED:		{ draw_state = CBS_CHECKEDDISABLED; } break;
				case GLYPH_RB_UNCHECKED_NORMAL:		{ draw_state = RBS_UNCHECKEDNORMAL; } break;
				case GLYPH_RB_CHECKED_NORMAL:		{ draw_state = RBS_CHECKEDNORMAL; } break;
				case GLYPH_RB_UNCHECKED_HOT:		{ draw_state = RBS_UNCHECKEDHOT; } break;
				case GLYPH_RB_CHECKED_HOT:			{ draw_state = RBS_CHECKEDHOT; } break;
				case GLYPH_RB_UNCHECKED_PRESSED:	{ draw_state = RBS_UNCHECKEDPRESSED; } break;
				case GLYPH_RB_CHECKED_PRESSED:		{ draw_state = RBS_CHECKEDPRESSED; } break;
				case GLYPH_RB_UNCHECKED_DISABLED:	{ draw_state = RBS_UNCHECKEDDISABLED; } break;
				case GLYPH_RB_CHECKED_DISABLED:		{ draw_state = RBS_CHECKEDDISABLED; } break;
				default:							{ draw_state = 0; } break;
			}

			SIZE button_size;
			_GetThemePartSize( ( HTHEME )glyphs, NULL, BP_CHECKBOX, draw_state, NULL, TS_DRAW, &button_size );

			RECT rc;
			rc.left = client_rc.left;
			rc.top = client_rc.top + ( ( height - button_size.cy ) / 2 );
			rc.right = rc.left + button_size.cx;
			rc.bottom = rc.top + button_size.cy;
			_DrawThemeBackground( ( HTHEME )glyphs, hdcMem, BP_CHECKBOX, draw_state, &rc, 0 );
		}
	}

	HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
	HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
	_SetTextColor( hdcMem, text_color );
	// Transparent background for text.
	_SetBkMode( hdcMem, TRANSPARENT );

	RECT rc;
	_memzero( &rc, sizeof( RECT ) );

	wchar_t *buf;
	wchar_t tbuf[ 128 ];
	int len = ( int )_SendMessageW( hWnd, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL character.
	if ( len > 128 )
	{
		buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
	}
	else
	{
		buf = tbuf;
	}

	if ( buf != NULL )
	{
		len = ( int )_SendMessageW( hWnd, WM_GETTEXT, len, ( LPARAM )buf );

		RECT text_rc;
		text_rc.left = client_rc.left;
		text_rc.right = client_rc.right;
		text_rc.top = client_rc.top;
		text_rc.bottom = client_rc.bottom;

		_DrawTextW( hdcMem, buf, len, &text_rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_CALCRECT );

		int text_height = text_rc.bottom - text_rc.top;
		int text_width = text_rc.right - text_rc.left;

		// Vertically center the text.
		rc.left = client_rc.left + _SCALE_DM_( 13 ) + _SCALE_DM_( 3 );
		rc.top = client_rc.top + ( ( height - text_height ) / 2 );
		rc.right = rc.left + text_width;
		rc.bottom = rc.top + text_height;

		_DrawTextW( hdcMem, buf, len, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );

		if ( buf != tbuf )
		{
			GlobalFree( buf );
		}
	}

	DWORD ui_state = ( DWORD )_SendMessageW( hWnd, WM_QUERYUISTATE, 0, 0 );
	if ( !( ui_state & UISF_HIDEFOCUS ) && hWnd == _GetFocus() )
	{
		--rc.top;
		++rc.bottom;
		--rc.left;
		++rc.right;
		//_DrawFocusRect( hdcMem, &rc );	// This doesn't look consistent.

		LOGBRUSH lb;
		lb.lbColor = dm_color_focus_rectangle;
		lb.lbStyle = PS_SOLID;
		HPEN hPen = _ExtCreatePen( PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, NULL );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );
		_Rectangle( hdcMem, rc.left, rc.top, rc.right, rc.bottom );
		_DeleteObject( hPen );
	}

	_SelectObject( hdcMem, ohf );

	// Draw our memory buffer to the main device context.
	_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

	// Delete our back buffer.
	_DeleteDC( hdcMem );
}

// Check Box/Radio Button Control
LRESULT CALLBACK DMCBRBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			// Only update if it's visible.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == FALSE )
				{
					state |= 0x10;	// Was enabled.
				}
				else
				{
					state |= 0x20;	// Was disabled.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}

			return 0;
		}
		break;

		case BM_SETCHECK:
		{
			// Make sure that our button has changed states so that we can update it.
			if ( ( UINT_PTR )_SendMessageW( hWnd, BM_GETCHECK, 0, 0 ) != wParam )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == BST_UNCHECKED )
				{
					state |= 0x40;	// Was checked
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_KILLFOCUS:
		case WM_SETFOCUS:
		case WM_SETTEXT:	// Occurs when changing proxy types.
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSELEAVE:
		{
			DWORD state;

			if ( _IsWindowVisible( hWnd ) == FALSE )	// We've tabbed away. Reset to the default state.
			{
				state = 0;
			}
			else
			{
				state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				state &= ~0x04;		// Unset the hovered state.

				state &= ~0x80;		// Not in bounds.
			}

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			state |= 0x08;	// Set the down state.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_CAPTURECHANGED:
		case WM_LBUTTONUP:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( state & 0x08 )
			{
				state &= ~0x08;	// Unset the down state.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			int xPos = GET_X_LPARAM( lParam ); 
			int yPos = GET_Y_LPARAM( lParam );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			int width = ( client_rc.right - client_rc.left );
			int height = ( client_rc.bottom - client_rc.top );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			DWORD old_state = state;

			bool bounds_changed = false;

			if ( xPos >= 0 && xPos <= width && yPos >= 0 && yPos <= height )
			{
				if ( !( state & 0x80 ) )
				{
					bounds_changed = true;
				}

				state |= 0x80;
			}
			else
			{
				if ( state & 0x80 )
				{
					bounds_changed = true;
				}

				state &= ~0x80;
			}

			// Is it currently hovered?
			if ( !( state & 0x04 ) )
			{
				state |= 0x04;	// Set the hovered state.
			}

			if ( state != old_state )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			HWND hWnd_parent = _GetParent( hWnd );

			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
			
			UINT last_dpi_dark_mode = ( UINT )_GetPropW( hWnd, L"DPI" );

			if ( current_dpi_dark_mode != 0 && last_dpi_dark_mode != 0 && current_dpi_dark_mode != last_dpi_dark_mode )
			{
				_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

				HANDLE glyphs = _GetPropW( hWnd_parent, L"GLYPHS" );

				unsigned char glyph_type = ( unsigned char )_GetPropW( hWnd, L"GLYPH_TYPE" );
				if ( glyph_type == 0 )
				{
					_ImageList_Destroy( ( HIMAGELIST )glyphs );
				}
				else if ( glyph_type == 1 )
				{
					_CloseThemeData( ( HTHEME )glyphs );
				}

				LONG height_width = 0;
				HIMAGELIST hIL = UpdateButtonGlyphs( hWnd, &height_width );
				if ( hIL != NULL )
				{
					_SetPropW( hWnd_parent, L"GLYPH_HW", ( HANDLE )height_width );

					_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hIL );

					_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )0 );	// Image List
				}
				else
				{
					HTHEME hTheme = _OpenThemeData( hWnd, L"Button" );

					_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hTheme );

					_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )1 );	// Theme
				}

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			if ( state == 0 )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );
			}

			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x100 ) )
			{
				state &= ~0x100;

				bool color_state1 = ( state & 0x01 ? true : false );	// hovered
				bool color_state2 = ( state & 0x02 ? true : false );	// down
				bool is_hovered =	( state & 0x04 ? true : false );
				bool is_down =		( state & 0x08 ? true : false );
				bool was_enabled =	( state & 0x10 ? true : false );
				bool was_disabled =	( state & 0x20 ? true : false );
				bool was_checked =	( state & 0x40 ? true : false );
				bool in_bounds =	( state & 0x80 ? true : false );

				int draw_state1 = GLYPH_CB_UNCHECKED_NORMAL;
				int draw_state2 = GLYPH_CB_UNCHECKED_NORMAL;

				COLORREF text_color1;
				COLORREF text_color2;

				if ( _IsWindowEnabled( hWnd ) != FALSE )
				{
					if ( is_down )
					{
						if ( in_bounds )
						{
							draw_state1 = GLYPH_CB_UNCHECKED_HOT;
							draw_state2 = GLYPH_CB_UNCHECKED_PRESSED;

							state |= 0x01;		// Turn on the hovered color state.
							state |= 0x02;		// Turn on the hovered color state.
						}
						else
						{
							if ( color_state2 )
							{
								draw_state1 = GLYPH_CB_UNCHECKED_PRESSED;
								//draw_state2 = GLYPH_CB_UNCHECKED_NORMAL;

								state &= ~0x01;	// Turn off the hovered color state.
								state &= ~0x02;	// Turn off the down color state.
							}
						}
					}
					else if ( is_hovered )
					{
						if ( color_state2 )
						{
							draw_state1 = GLYPH_CB_UNCHECKED_PRESSED;
							draw_state2 = GLYPH_CB_UNCHECKED_HOT;

							state &= ~0x02;		// Turn off the down color state.
						}
						else
						{
							//draw_state1 = GLYPH_CB_UNCHECKED_NORMAL;
							draw_state2 = GLYPH_CB_UNCHECKED_HOT;

							state |= 0x01;		// Turn on the hovered color state.
						}
					}
					else
					{
						if ( color_state2 )
						{
							draw_state1 = GLYPH_CB_UNCHECKED_PRESSED;
							//draw_state2 = GLYPH_CB_UNCHECKED_NORMAL;

							state &= ~0x01;	// Turn off the hovered color state.
							state &= ~0x02;	// Turn off the down color state.
						}
						else if ( color_state1 )
						{
							draw_state1 = GLYPH_CB_UNCHECKED_HOT;
							//draw_state2 = GLYPH_CB_UNCHECKED_NORMAL;

							state &= ~0x01;	// Turn off the hovered color state.
						}
					}

					text_color1 = text_color2 = dm_color_window_text;

					if ( was_disabled )
					{
						draw_state1 = GLYPH_CB_UNCHECKED_DISABLED;

						text_color1 = _GetSysColor( COLOR_GRAYTEXT );

						state &= ~0x20;
					}
					else if ( was_checked )
					{
						++draw_state1;	// Checked Normal
						draw_state2 = GLYPH_CB_UNCHECKED_NORMAL;

						state &= ~0x40;
					}
				}
				else
				{
					if ( was_enabled )
					{
						//draw_state1 = GLYPH_CB_UNCHECKED_NORMAL;

						text_color1 = dm_color_window_text;

						state &= ~0x10;
					}
					else
					{
						draw_state1 = GLYPH_CB_UNCHECKED_DISABLED;

						text_color1 = _GetSysColor( COLOR_GRAYTEXT );
					}

					// Check was toggled while disabled.
					if ( was_checked )
					{
						state &= ~0x40;
					}

					draw_state2 = GLYPH_CB_UNCHECKED_DISABLED;

					text_color2 = _GetSysColor( COLOR_GRAYTEXT );
				}

				if ( _SendMessageW( hWnd, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
				{
					++draw_state1;
					++draw_state2;
				}

				LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
				if ( ( style & 0x0000000F ) == BS_AUTORADIOBUTTON )
				{
					draw_state1 += ( GLYPH_RB_UNCHECKED_NORMAL - 1 );
					draw_state2 += ( GLYPH_RB_UNCHECKED_NORMAL - 1 );
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				if ( draw_state1 != draw_state2 || text_color1 != text_color2 )
				{
					BP_ANIMATIONPARAMS animParams;
					_memzero( &animParams, sizeof( animParams ) );
					animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
					animParams.style = BPAS_LINEAR;
					animParams.dwDuration = CBRB_ANIMATION_DURATION;

					RECT client_rc;
					_GetClientRect( hWnd, &client_rc );

					HDC hdcFrom, hdcTo;
					HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &client_rc, BPBF_COMPATIBLEBITMAP, NULL, &animParams, &hdcFrom, &hdcTo );
					if ( hbpAnimation )
					{
						if ( hdcFrom )
						{
							CBRBPaint( hWnd, hdcFrom, text_color1, draw_state1 );
						}

						if ( hdcTo )
						{
							CBRBPaint( hWnd, hdcTo, text_color2, draw_state2 );
						}

						_EndBufferedAnimation( hbpAnimation, TRUE );
					}
					else
					{
						CBRBPaint( hWnd, hDC, text_color1, draw_state1 );
					}
				}
				else
				{
					CBRBPaint( hWnd, hDC, text_color1, draw_state1 );
				}
			}

			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMCBRBSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

// GroupBox control
LRESULT CALLBACK DMGBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			HWND hWnd_parent = _GetParent( hWnd );
			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			HBRUSH color = _CreateSolidBrush( dm_color_window_background );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_window_border );
			HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
			_DeleteObject( old_color );
			HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
			_DeleteObject( old_brush );
			_Rectangle( hdcMem, client_rc.left, client_rc.top + _SCALE_DM_( 7 ), client_rc.right, client_rc.bottom );
			_DeleteObject( hPen );

			HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
			HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
			if ( _IsWindowEnabled( hWnd ) == TRUE )
			{
				_SetTextColor( hdcMem, dm_color_window_text );
			}
			else
			{
				_SetTextColor( hdcMem, _GetSysColor( COLOR_GRAYTEXT ) );
			}
			_SetBkMode( hdcMem, OPAQUE );
			_SetBkColor( hdcMem, dm_color_window_background );

			RECT rc;
			rc.left = client_rc.left + _SCALE_DM_( 6 );
			rc.top = client_rc.top;
			rc.right = client_rc.right;
			rc.bottom = client_rc.bottom;

			wchar_t *buf;
			wchar_t tbuf[ 128 ];
			int len = ( int )_SendMessageW( hWnd, WM_GETTEXTLENGTH, 0, 0 ) + 3;	// Include the NULL character.
			if ( len > 128 )
			{
				buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
			}
			else
			{
				buf = tbuf;
			}

			if ( buf != NULL )
			{
				buf[ 0 ] = L' ';
				len = ( int )_SendMessageW( hWnd, WM_GETTEXT, len - 2, ( LPARAM )( buf + 1 ) ) + 2;
				buf[ len - 1 ] = L' ';
				buf[ len ] = 0; // Sanity.

				_DrawTextW( hdcMem, buf, len, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT );
				_DrawTextW( hdcMem, buf, len, &rc, DT_NOPREFIX | DT_SINGLELINE );

				if ( buf != tbuf )
				{
					GlobalFree( buf );
				}
			}

			_SelectObject( hdcMem, ohf );

			_ExcludeClipRect( hDC, 1, client_rc.top + rc.bottom + 1, client_rc.right - 1, client_rc.bottom - 1 );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMGBSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void UDPaint( HWND hWnd, HDC hDC, COLORREF arrow_color, COLORREF button1_color, COLORREF button2_color, COLORREF border_color )
{
	RECT client_rc;
	_GetClientRect( hWnd, &client_rc );

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
	_DeleteObject( ohbm );
	_DeleteObject( hbm );

	// Button 1
	RECT rc_btn1;
	rc_btn1.left = client_rc.left + 1;
	rc_btn1.top = client_rc.top + 2;
	rc_btn1.right = client_rc.right - 2;
	rc_btn1.bottom = ( client_rc.bottom - client_rc.top ) / 2;

	// Button 2
	RECT rc_btn2;
	rc_btn2.left = rc_btn1.left;
	rc_btn2.right = rc_btn1.right;
	rc_btn2.top = rc_btn1.bottom + 1;
	rc_btn2.bottom = client_rc.bottom - 2;

	HBRUSH color = _CreateSolidBrush( dm_color_button_normal );
	_FillRect( hdcMem, &client_rc, color );
	_DeleteObject( color );

	HPEN hPen = _CreatePen( PS_SOLID, 1, border_color );
	HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
	_DeleteObject( old_color );
	HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
	_DeleteObject( old_brush );

	_MoveToEx( hdcMem, client_rc.left, client_rc.top, NULL );
	_LineTo( hdcMem, client_rc.right - 1, client_rc.top );
	_LineTo( hdcMem, client_rc.right - 1, client_rc.bottom - 1 );
	_LineTo( hdcMem, client_rc.left - 1, client_rc.bottom - 1 );

	color = _CreateSolidBrush( button1_color );
	_FillRect( hdcMem, &rc_btn1, color );
	_DeleteObject( color );

	color = _CreateSolidBrush( button2_color );
	_FillRect( hdcMem, &rc_btn2, color );
	_DeleteObject( color );

	_Rectangle( hdcMem, rc_btn1.left, rc_btn1.top, rc_btn1.right, rc_btn1.bottom );
	_Rectangle( hdcMem, rc_btn2.left, rc_btn2.top, rc_btn2.right, rc_btn2.bottom );

	_DeleteObject( hPen );

	LONG a_left = ( ( client_rc.right - client_rc.left ) - 5 ) / 2;

	LONG ua_top = rc_btn1.top + ( ( ( rc_btn1.bottom - rc_btn1.top ) - 3 ) / 2 );
	POINT u_arrow[ 3 ];
	u_arrow[ 0 ].x = a_left + 2;
	u_arrow[ 0 ].y = ua_top;
	u_arrow[ 1 ].x = a_left;
	u_arrow[ 1 ].y = ua_top + 2;
	u_arrow[ 2 ].x = a_left + 4;
	u_arrow[ 2 ].y = ua_top + 2;

	LONG da_top = rc_btn2.top + ( ( ( rc_btn2.bottom - rc_btn2.top ) - 3 ) / 2 );
	POINT d_arrow[ 3 ];
	d_arrow[ 0 ].x = a_left;
	d_arrow[ 0 ].y = da_top;
	d_arrow[ 1 ].x = a_left + 2;
	d_arrow[ 1 ].y = da_top + 2;
	d_arrow[ 2 ].x = a_left + 4;
	d_arrow[ 2 ].y = da_top;
	
	hPen = _CreatePen( PS_SOLID, 1, arrow_color );
	old_color = ( HPEN )_SelectObject( hdcMem, hPen );
	_DeleteObject( old_color );
	color = _CreateSolidBrush( arrow_color );
	old_brush = ( HBRUSH )_SelectObject( hdcMem, color );
	_DeleteObject( old_brush );
	_Polygon( hdcMem, u_arrow, 3 );
	_Polygon( hdcMem, d_arrow, 3 );
	_DeleteObject( color );
	_DeleteObject( hPen );

	// Draw our memory buffer to the main device context.
	_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

	// Delete our back buffer.
	_DeleteDC( hdcMem );
}

// Up/Down control
LRESULT CALLBACK DMUDSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			// Only update if it's visible.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == FALSE )
				{
					state |= 0x400;	// Was enabled
				}
				else
				{
					state |= 0x800;	// Was disabled.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}

			return 0;
		}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x1000 ) );

			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			state &= ~0x08;		// Unset the down state.

			state &= ~0x80;		// Unset the down state.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_LBUTTONDOWN:
		{
			int yPos = GET_Y_LPARAM( lParam );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			int height = ( client_rc.bottom - client_rc.top ) / 2;

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( yPos < height )	// Button 1 down, button 2 up
			{
				state |= 0x08;		// Set the down state.
				state &= ~0x80;		// Unset the down state.
			}
			else if ( yPos > height )	// Button 2 down, button 1 up
			{
				state |= 0x80;		// Set the down state.
				state &= ~0x08;		// Unset the down state.
			}
			else
			{
				state &= ~0x08;		// Unset the down state.
				state &= ~0x80;		// Unset the down state.
			}

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSELEAVE:
		{
			DWORD state;

			if ( _IsWindowVisible( hWnd ) == FALSE )	// We've tabbed away. Reset to the default state.
			{
				state = 0;
			}
			else
			{
				state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				state &= ~0x04;			// Unset the hovered state.

				state &= ~0x40;			// Unset the hovered state.

				if ( !( state & 0x100 ) )
				{
					state |= 0x200;		// Was in bounds.
				}
				else
				{
					state &= ~0x100;	// Not in bounds.
				}
			}

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSEMOVE:
		{
			int xPos = GET_X_LPARAM( lParam ); 
			int yPos = GET_Y_LPARAM( lParam );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			int width = ( client_rc.right - client_rc.left );
			int height = ( client_rc.bottom - client_rc.top );
			int button_height = height / 2;

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			DWORD old_state = state;

			if ( xPos >= 0 && xPos <= width && yPos >= 0 && yPos <= height )
			{
				state |= 0x100;
			}
			else
			{
				state &= ~0x100;
			}

			if ( yPos < button_height )
			{
				// Is button 1 not currently hovered?
				if ( !( state & 0x04 ) )
				{
					// Second button was hovered over.
					if ( state & 0x40 )
					{
						state &= ~0x40;	// Unset the hovered state.
					}

					state |= 0x04;	// Set the hovered state.
				}
			}
			else if ( yPos > button_height )
			{
				// Is button 2 not currently hovered?
				if ( !( state & 0x40 ) )
				{
					// First button was hovered over.
					if ( state & 0x04 )
					{
						state &= ~0x04;	// Unset the hovered state.
					}

					state |= 0x40;	// Set the hovered state.
				}
			}
			else
			{
				state &= ~0x04;	// Unset the hovered state.

				state &= ~0x40;	// Unset the hovered state.
			}

			if ( state != old_state )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			if ( state == 0 )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x1000 ) );
			}

			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x1000 ) )
			{
				state &= ~0x1000;

				COLORREF buttons[ 4 ];

				buttons[ 0 ] = dm_color_button_normal;
				buttons[ 1 ] = dm_color_button_normal;
				buttons[ 2 ] = dm_color_button_normal;
				buttons[ 3 ] = dm_color_button_normal;

				COLORREF arrow_color1;
				COLORREF arrow_color2;

				COLORREF border_color1;
				COLORREF border_color2;

				bool in_bounds = ( state & 0x100 ? true : false );
				bool was_in_bounds = ( state & 0x200 ? true : false );
				bool was_enabled =	( state & 0x400 ? true : false );
				bool was_disabled =	( state & 0x800 ? true : false );

				state &= ~0x200;
				state &= ~0x400;
				state &= ~0x800;

				for ( char i = 0; i < 2; ++i )
				{
					bool color_state1 = ( state & ( 0x01 << ( i * 4 ) ) ? true : false );	// hovered
					bool color_state2 = ( state & ( 0x02 << ( i * 4 ) ) ? true : false );	// down
					bool is_hovered =	( state & ( 0x04 << ( i * 4 ) ) ? true : false );
					bool is_down =		( state & ( 0x08 << ( i * 4 ) ) ? true : false );

					if ( is_down )
					{
						if ( !color_state2 )
						{
							if ( !color_state1 )
							{
								//buttons[ 0 + ( i * 2 ) ] = dm_color_button_normal;
								buttons[ 1 + ( i * 2 ) ] = dm_color_button_pressed;

								state |= ( 0x01 << ( i * 4 ) );	// Turn on the hovered color state.
							}
							else
							{
								buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
								buttons[ 1 + ( i * 2 ) ] = dm_color_button_pressed;
							}

							state |= ( 0x02 << ( i * 4 ) );	// Turn on the down color state.
						}
						else
						{
							if ( !is_hovered )
							{
								if ( color_state1 )
								{
									if ( in_bounds )
									{
										buttons[ 0 + ( i * 2 ) ] = dm_color_button_pressed;
										//buttons[ 1 + ( i * 2 ) ] = dm_color_button_normal;
									}
									else
									{
										buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
										//buttons[ 1 + ( i * 2 ) ] = dm_color_button_normal;
									}
								}

								state &= ~( 0x01 << ( i * 4 ) );	// Turn off the hovered color state.
							}
							else
							{
								if ( in_bounds )
								{
									buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
									buttons[ 1 + ( i * 2 ) ] = dm_color_button_pressed;
								}
								else
								{
									if ( !color_state1 )
									{
										//buttons[ 0 + ( i * 2 ) ] = dm_color_button_normal;
										buttons[ 1 + ( i * 2 ) ] = dm_color_button_hover;
									}
									else
									{
										buttons[ 0 + ( i * 2 ) ] = dm_color_button_pressed;
										buttons[ 1 + ( i * 2 ) ] = dm_color_button_hover;
									}
								}

								state |= ( 0x01 << ( i * 4 ) );	// Turn on the hovered color state.
							}
						}
					}
					else if ( is_hovered )
					{
						if ( !color_state1 )
						{
							//buttons[ 0 + ( i * 2 ) ] = dm_color_button_normal;
							buttons[ 1 + ( i * 2 ) ] = dm_color_button_hover;

							state |= ( 0x01 << ( i * 4 ) );	// Turn on the hovered color state.
						}
						else
						{
							if ( color_state2 )
							{
								if ( in_bounds )
								{
									buttons[ 0 + ( i * 2 ) ] = dm_color_button_pressed;
									buttons[ 1 + ( i * 2 ) ] = dm_color_button_hover;

									state |= ( 0x01 << ( i * 4 ) );		// Turn on the hovered color state.
									state &= ~( 0x02 << ( i * 4 ) );	// Turn off the down color state.
								}
								else
								{
									buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
									//buttons[ 1 + ( i * 2 ) ] = dm_color_button_normal;

									state &= ~( 0x01 << ( i * 4 ) );	// Turn off the hovered color state.
									state &= ~( 0x02 << ( i * 4 ) );	// Turn off the down color state.
								}
							}
							else
							{
								buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
								buttons[ 1 + ( i * 2 ) ] = dm_color_button_hover;
							}
						}
					}
					else
					{
						if ( color_state2 )	// We held the mouse down on a button and then moved to another button and released the mouse.
						{
							state &= ~( 0x01 << ( i * 4 ) );	// Turn off the hovered color state.
							state &= ~( 0x02 << ( i * 4 ) );	// Turn off the down color state.
						}
						else if ( color_state1 )
						{
							buttons[ 0 + ( i * 2 ) ] = dm_color_button_hover;
							//buttons[ 1 + ( i * 2 ) ] = dm_color_button_normal;

							state &= ~( 0x01 << ( i * 4 ) );	// Turn off the hovered color state.
						}
					}
				}

				if ( _IsWindowEnabled( hWnd ) != FALSE )
				{
					arrow_color2 = dm_color_arrow;
					border_color2 = dm_color_ud_button_border_enabled;
				}
				else
				{
					arrow_color2 = _GetSysColor( COLOR_GRAYTEXT );
					border_color2 = dm_color_ud_button_border_disabled;
				}

				if ( was_enabled )
				{
					arrow_color1 = dm_color_arrow;
					border_color1 = dm_color_ud_button_border_enabled;
				}
				else if ( was_disabled )
				{
					arrow_color1 = _GetSysColor( COLOR_GRAYTEXT );
					border_color1 = dm_color_ud_button_border_disabled;
				}
				else
				{
					arrow_color1 = arrow_color2;
					border_color1 = border_color2;
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				if ( was_in_bounds || was_enabled || was_disabled || buttons[ 0 ] != buttons[ 1 ] || buttons[ 2 ] != buttons[ 3 ] )
				{
					BP_ANIMATIONPARAMS animParams;
					_memzero( &animParams, sizeof( animParams ) );
					animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
					animParams.style = BPAS_LINEAR;
					animParams.dwDuration = ( was_enabled || was_disabled ? BUTTON_ANIMATION_DURATION / 2 : BUTTON_ANIMATION_DURATION );

					RECT client_rc;
					_GetClientRect( hWnd, &client_rc );

					HDC hdcFrom, hdcTo;
					HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &client_rc, BPBF_COMPATIBLEBITMAP, NULL, &animParams, &hdcFrom, &hdcTo );
					if ( hbpAnimation )
					{
						if ( hdcFrom )
						{
							UDPaint( hWnd, hdcFrom, arrow_color1, buttons[ 0 ], buttons[ 2 ], border_color1 );
						}

						if ( hdcTo )
						{
							UDPaint( hWnd, hdcTo, arrow_color2, buttons[ 1 ], buttons[ 3 ], border_color2 );
						}

						_EndBufferedAnimation( hbpAnimation, TRUE );
					}
					else
					{
						UDPaint( hWnd, hDC, arrow_color1, buttons[ 0 ], buttons[ 2 ], border_color1 );
					}
				}
				else
				{
					UDPaint( hWnd, hDC, arrow_color1, buttons[ 0 ], buttons[ 2 ], border_color1 );
				}
			}

			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMUDSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void ComboBoxPaint( HWND hWnd, HDC hDC, COLORREF text_color, COLORREF arrow_color, COLORREF background_color, COLORREF border_color )
{
	HWND hWnd_parent = _GetParent( hWnd );
	UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

	RECT client_rc;
	_GetClientRect( hWnd, &client_rc );

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
	_DeleteObject( ohbm );
	_DeleteObject( hbm );

	HBRUSH color = _CreateSolidBrush( background_color );
	_FillRect( hdcMem, &client_rc, color );
	_DeleteObject( color );

	LONG a_left = ( client_rc.right - _SCALE_DM_( 18 ) ) + ( ( _SCALE_DM_( 18 ) - _SCALE_DM_( 9 ) ) / 2 );
	LONG da_top = ( ( ( client_rc.bottom - client_rc.top ) - _SCALE_DM_( 4 ) ) / 2 );
	POINT d_arrow[ 3 ];
	d_arrow[ 0 ].x = a_left;
	d_arrow[ 0 ].y = da_top;
	d_arrow[ 1 ].x = a_left + _SCALE_DM_( 4 );
	d_arrow[ 1 ].y = da_top + _SCALE_DM_( 4 );
	d_arrow[ 2 ].x = a_left + _SCALE_DM_( 8 );
	d_arrow[ 2 ].y = da_top;

	RECT rc;

	LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
	if ( ( style & 0x0000000F ) == CBS_DROPDOWN )
	{
		HPEN hPen = _CreatePen( PS_SOLID, 1, border_color );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );
		_Rectangle( hdcMem, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom );
		_DeleteObject( hPen );

		hPen = _CreatePen( PS_SOLID, 1, arrow_color );
		old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		color = _CreateSolidBrush( arrow_color );
		old_brush = ( HBRUSH )_SelectObject( hdcMem, color );
		_DeleteObject( old_brush );
		_Polygon( hdcMem, d_arrow, 3 );
		_DeleteObject( color );
		_DeleteObject( hPen );

		rc.left = client_rc.left;
		rc.top = client_rc.top;
		rc.right = client_rc.right - _SCALE_DM_( 18 );
		rc.bottom = client_rc.bottom;

		color = _CreateSolidBrush( dm_color_edit_background );
		_FillRect( hdcMem, &rc, color );
		_DeleteObject( color );

		hPen = _CreatePen( PS_SOLID, 1, border_color );
		old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );
		_Rectangle( hdcMem, rc.left, rc.top, rc.right, rc.bottom );
		_DeleteObject( hPen );
	}
	else
	{
		HPEN hPen = _CreatePen( PS_SOLID, 1, border_color );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );
		_Rectangle( hdcMem, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom );
		_DeleteObject( hPen );

		hPen = _CreatePen( PS_SOLID, 1, arrow_color );
		old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		color = _CreateSolidBrush( arrow_color );
		old_brush = ( HBRUSH )_SelectObject( hdcMem, color );
		_DeleteObject( old_brush );
		_Polygon( hdcMem, d_arrow, 3 );
		_DeleteObject( color );
		_DeleteObject( hPen );

		HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
		HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
		_SetTextColor( hdcMem, text_color );
		// Transparent background for text.
		_SetBkMode( hdcMem, TRANSPARENT );

		wchar_t buf[ 128 ];
		int index = ( int )_SendMessageW( hWnd, CB_GETCURSEL, 0, 0 );
		int len = ( int )_SendMessageW( hWnd, CB_GETLBTEXT, index, ( LPARAM )buf );

		RECT text_rc;
		text_rc.left = client_rc.left;
		text_rc.right = client_rc.right;
		text_rc.top = client_rc.top;
		text_rc.bottom = client_rc.bottom;
		_DrawTextW( hdcMem, buf, len, &text_rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_CALCRECT );

		int text_height = text_rc.bottom - text_rc.top;
		int text_width = text_rc.right - text_rc.left;

		// Vertically center the text.
		rc.left = client_rc.left + _SCALE_DM_( 4 );
		rc.top = client_rc.top + ( ( ( client_rc.bottom - client_rc.top ) - text_height ) / 2 );
		rc.right = rc.left + text_width;
		rc.bottom = rc.top + text_height;

		_DrawTextW( hdcMem, buf, len, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );

		if ( _SendMessageW( hWnd, CB_GETDROPPEDSTATE, 0, 0 ) == FALSE )
		{
			DWORD ui_state = ( DWORD )_SendMessageW( hWnd, WM_QUERYUISTATE, 0, 0 );
			if ( !( ui_state & UISF_HIDEFOCUS ) && hWnd == _GetFocus() )
			{
				rc.top = rc.top - _SCALE_DM_( 1 );
				rc.bottom = rc.bottom + _SCALE_DM_( 1 );
				rc.left = rc.left - _SCALE_DM_( 1 );
				rc.right = client_rc.right - _SCALE_DM_( 20 );

				LOGBRUSH lb;
				lb.lbColor = dm_color_focus_rectangle;
				lb.lbStyle = PS_SOLID;
				hPen = _ExtCreatePen( PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, NULL );
				old_color = ( HPEN )_SelectObject( hdcMem, hPen );
				_DeleteObject( old_color );
				old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
				_DeleteObject( old_brush );
				_Rectangle( hdcMem, rc.left, rc.top, rc.right, rc.bottom );
				_DeleteObject( hPen );
			}
		}

		_SelectObject( hdcMem, ohf );
	}

	// Draw our memory buffer to the main device context.
	_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

	// Delete our back buffer.
	_DeleteDC( hdcMem );
}

LRESULT CALLBACK DMComboBoxSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			// Only update if it's visible.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == FALSE )
				{
					state |= 0x10;	// Was enabled
				}
				else
				{
					state |= 0x20;	// Was disabled.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}

			return 0;
		}
		break;

		case CB_SETCURSEL:
		case WM_KEYDOWN:	// Handles up/down and mouse wheel events.
		case WM_SETFOCUS:
		{
			_BufferedPaintStopAllAnimations( hWnd );

			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_KILLFOCUS:
		{
			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( ( style & 0x0000000F ) != CBS_DROPDOWN )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				// This condition handles the case where the control is focused and we switch to a different tab.
				// If we switch back, the previously focused control would have animated. We don't want that to happen.
				if ( _IsWindowVisible( hWnd ) != FALSE )
				{
					state |= 0x40;	// Was focused.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );
			}
		}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );

			//return 0;
		}
		break;

		case WM_PARENTNOTIFY:
		{
			switch ( wParam )
			{
				case WM_KILLFOCUS:
				{
					_BufferedPaintStopAllAnimations( hWnd );

					DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

					// This condition handles the case where the control is focused and we switch to a different tab.
					// If we switch back, the previously focused control would have animated. We don't want that to happen.
					if ( _IsWindowVisible( hWnd ) != FALSE )
					{
						state |= 0x40;	// Was focused.
					}

					_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );
				}
				break;

				case WM_MOUSELEAVE:
				{
					_SendMessageW( hWnd, WM_MOUSELEAVE, 0, 0 );
				}
				break;

				case WM_MOUSEMOVE:
				{
					_SendMessageW( hWnd, WM_MOUSEMOVE, 0, 0 );
				}
				break;
			}
			break;
		}
		break;

		case WM_WINDOWPOSCHANGING:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( state & 0x08 )
			{
				state |= 0x80;	// Menu was opened before we clicked.
			}

			POINT pt;
			_GetCursorPos( &pt );
			RECT rc;
			_GetWindowRect( hWnd, &rc );

			if ( pt.x < rc.left || pt.x > rc.right || pt.y < rc.top || pt.y > rc.bottom )
			{
				state &= ~0x04;		// Unset the hovered state.
			}

			state &= ~0x08;		// Unset the down state.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSELEAVE:
		{
			POINT pt;
			_GetCursorPos( &pt );
			RECT rc;
			_GetClientRect( hWnd, &rc );
			_MapWindowPoints( HWND_DESKTOP, hWnd, &pt, 1 );

			if ( pt.x <= rc.left || pt.x >= rc.right || pt.y <= rc.top || pt.y >= rc.bottom )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				state &= ~0x04;		// Unset the hovered state.

				state &= ~0x08;		// Unset the down state.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
			else if ( _IsWindowVisible( hWnd ) == FALSE )	// We've tabbed away. Reset to the default state.
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )0x100 );
			}
		}
		break;

		case WM_LBUTTONDOWN:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			state ^= 0x08;	// Set/Unset the down state.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSEMOVE:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			// Is it currently hovered?
			if ( !( state & 0x04 ) )
			{
				state |= 0x04;	// Set the hovered state.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			HWND hWnd_parent = _GetParent( hWnd );

			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
			
			UINT last_dpi_dark_mode = ( UINT )_GetPropW( hWnd, L"DPI" );

			if ( current_dpi_dark_mode != 0 && last_dpi_dark_mode != 0 && current_dpi_dark_mode != last_dpi_dark_mode )
			{
				_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

				int height = ( int )_SendMessageW( hWnd, CB_GETITEMHEIGHT, ( WPARAM )-1, 0 );
				if ( height != CB_ERR )
				{
					_SendMessageW( hWnd, CB_SETITEMHEIGHT, ( WPARAM )-1, _SCALE2_( height, dpi_dark_mode ) );	// This isn't documented? Adjusts the box's height.
				}

				height = ( int )_SendMessageW( hWnd, CB_GETITEMHEIGHT, 0, 0 );
				if ( height != CB_ERR )
				{
					_SendMessageW( hWnd, CB_SETITEMHEIGHT, 0, _SCALE2_( height, dpi_dark_mode ) );
				}
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( state == 0 )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x100 ) );
			}

			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x100 ) )
			{
				state &= ~0x100;

				bool color_state1 = ( state & 0x01 ? true : false );	// hovered
				bool color_state2 = ( state & 0x02 ? true : false );	// down
				bool is_hovered =	( state & 0x04 ? true : false );
				//bool is_menu_open =	( state & 0x08 ? true : false );
				bool was_enabled =	( state & 0x10 ? true : false );
				bool was_disabled =	( state & 0x20 ? true : false );
				bool was_focused =	( state & 0x40 ? true : false );
				bool was_menu_opened = ( state & 0x80 ? true : false );

				state &= ~0x10;
				state &= ~0x20;
				state &= ~0x40;
				state &= ~0x80;

				HWND focused_hWnd;

				LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
				if ( ( style & 0x0000000F ) == CBS_DROPDOWN )
				{
					COMBOBOXINFO cbi;
					cbi.cbSize = sizeof( COMBOBOXINFO );
					_SendMessageW( hWnd, CB_GETCOMBOBOXINFO, 0, ( LPARAM )&cbi );
					focused_hWnd = cbi.hwndItem;
				}
				else
				{
					focused_hWnd = hWnd;
				}

				bool is_focused = ( focused_hWnd == _GetFocus() ? true : false );

				COLORREF background_color1 = dm_color_button_normal;
				COLORREF background_color2 = dm_color_button_normal;

				COLORREF border_color1 = dm_color_button_border_enabled;
				COLORREF border_color2 = dm_color_button_border_enabled;

				COLORREF arrow_color1;
				COLORREF arrow_color2;
				COLORREF text_color1;
				COLORREF text_color2;

				if ( _SendMessageW( hWnd, CB_GETDROPPEDSTATE, 0, 0 ) != FALSE )
				{
					if ( was_focused || color_state2 )
					{
						border_color1 = dm_color_focus_rectangle;
					}
					else if ( color_state1 )
					{
						border_color1 = dm_color_button_border_hover;
					}
					/*else
					{
						border_color1 = dm_color_button_border_enabled;
					}*/

					if ( is_focused || color_state2 )
					{
						border_color2 = dm_color_focus_rectangle;
					}
					else if ( color_state1 )
					{
						border_color2 = dm_color_button_border_hover;
					}
					/*else
					{
						border_color2 = dm_color_button_border_enabled;
					}*/

					if ( color_state2 )
					{
						background_color1 = dm_color_button_pressed;
					}
					else if ( color_state1 )
					{
						background_color1 = dm_color_button_hover;
					}
					/*else
					{
						background_color1 = dm_color_button_normal;
					}*/

					background_color2 = dm_color_button_pressed;

					state |= 0x01;		// Turn on the hovered color state.
					state |= 0x02;		// Turn on the down color state.
				}
				else if ( is_hovered )
				{
					if ( is_focused )
					{
						border_color1 = dm_color_focus_rectangle;

						if ( ( style & 0x0000000F ) == CBS_DROPDOWN )
						{
							border_color2 = dm_color_focus_rectangle;		// Similar to an edit control.
						}
						else
						{
							border_color2 = dm_color_button_border_hover;	// Similar to a button.
						}

						if ( was_menu_opened )
						{
							background_color1 = dm_color_button_pressed;
							background_color2 = dm_color_button_hover;

							state &= ~0x02;		// Turn off the down color state.
						}
						else
						{
							if ( color_state2 )
							{
								background_color1 = dm_color_button_pressed;
							}
							else if ( color_state1 )
							{
								background_color1 = dm_color_button_hover;
							}
							/*else
							{
								background_color1 = dm_color_button_normal;
							}*/
							
							background_color2 = dm_color_button_hover;
						}

						state |= 0x01;		// Turn on the hovered color state.
					}
					else if ( color_state2 )
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;

							background_color1 = dm_color_button_pressed;
						}
						else
						{
							border_color1 = dm_color_button_border_hover;

							background_color1 = dm_color_button_hover;
						}

						border_color2 = dm_color_button_border_hover;

						background_color2 = dm_color_button_hover;

						state &= ~0x02;		// Turn off the down color state.
					}
					else
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;

							background_color1 = dm_color_button_hover;
						}
						else if ( color_state1 )
						{
							border_color1 = dm_color_button_border_hover;

							background_color1 = dm_color_button_hover;
						}
						/*else
						{
							border_color1 = dm_color_button_border_enabled;

							background_color1 = dm_color_button_normal;
						}*/

						border_color2 = dm_color_button_border_hover;

						background_color2 = dm_color_button_hover;

						state |= 0x01;		// Turn on the hovered color state.
					}
				}
				else
				{
					if ( color_state2 )
					{
						background_color1 = dm_color_button_pressed;
						//background_color2 = dm_color_button_normal;

						state &= ~0x01;	// Turn off the hovered color state.
						state &= ~0x02;	// Turn off the down color state.
					}
					else if ( color_state1 )
					{
						border_color1 = dm_color_button_border_hover;
						//border_color2 = dm_color_button_border_enabled;

						background_color1 = dm_color_button_hover;
						//background_color2 = dm_color_button_normal;

						state &= ~0x01;	// Turn off the hovered color state.
					}

					if ( is_focused )
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;
						}
						else if ( color_state1 )
						{
							border_color1 = dm_color_button_border_hover;
						}
						/*else
						{
							border_color1 = dm_color_button_border_enabled;
						}*/

						border_color2 = dm_color_focus_rectangle;
					}
					else if ( was_focused )
					{
						border_color1 = dm_color_focus_rectangle;
						//border_color2 = dm_color_button_border_enabled;
					}
				}

				if ( _IsWindowEnabled( hWnd ) != FALSE )
				{
					if ( was_disabled )
					{
						border_color1 = dm_color_button_border_disabled;
						arrow_color1 = _GetSysColor( COLOR_GRAYTEXT );
						text_color1 = _GetSysColor( COLOR_GRAYTEXT );
					}
					else
					{
						arrow_color1 = dm_color_arrow;
						text_color1 = dm_color_window_text;
					}

					arrow_color2 = dm_color_arrow;
					text_color2 = dm_color_window_text;
				}
				else
				{
					if ( was_enabled )
					{
						arrow_color1 = dm_color_arrow;
						text_color1 = dm_color_window_text;
					}
					else
					{
						border_color1 = dm_color_button_border_disabled;
						arrow_color1 = _GetSysColor( COLOR_GRAYTEXT );
						text_color1 = _GetSysColor( COLOR_GRAYTEXT );
					}

					border_color2 = dm_color_button_border_disabled;
					arrow_color2 = _GetSysColor( COLOR_GRAYTEXT );
					text_color2 = _GetSysColor( COLOR_GRAYTEXT );
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				BP_ANIMATIONPARAMS animParams;
				_memzero( &animParams, sizeof( animParams ) );
				animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
				animParams.style = BPAS_LINEAR;
				animParams.dwDuration = BUTTON_ANIMATION_DURATION;

				BP_PAINTPARAMS paintParams;
				_memzero( &paintParams, sizeof( BP_PAINTPARAMS ) );
				paintParams.cbSize = sizeof( BP_PAINTPARAMS );
				paintParams.dwFlags = BPPF_ERASE;

				// Only animate the border.
				if ( !was_focused && !was_enabled && !was_disabled && background_color1 == background_color2 )
				{
					ComboBoxPaint( hWnd, hDC, text_color1, arrow_color1, background_color1, border_color1 );

					if ( border_color1 == border_color2 )
					{
						_EndPaint( hWnd, &ps );

						return 0;
					}

					RECT rc_exclude;
					rc_exclude.top = ps.rcPaint.top + 1;
					rc_exclude.left = ps.rcPaint.left + 1;
					if ( ( style & 0x0000000F ) == CBS_DROPDOWN )
					{
						rc_exclude.right = ps.rcPaint.right - ( 18 + 1 ); // Width of the button + frame.
					}
					else
					{
						rc_exclude.right = ps.rcPaint.right - 1;
					}
					rc_exclude.bottom = ps.rcPaint.bottom - 1;

					paintParams.prcExclude = &rc_exclude;
				}

				HDC hdcFrom, hdcTo;
				HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, &paintParams, &animParams, &hdcFrom, &hdcTo );
				if ( hbpAnimation )
				{
					if ( hdcFrom )
					{
						ComboBoxPaint( hWnd, hdcFrom, text_color1, arrow_color1, background_color1, border_color1 );
					}

					if ( hdcTo )
					{
						ComboBoxPaint( hWnd, hdcTo, text_color2, arrow_color2, background_color2, border_color2 );
					}

					_EndBufferedAnimation( hbpAnimation, TRUE );
				}
				else
				{
					ComboBoxPaint( hWnd, hDC, dm_color_window_text, dm_color_arrow, dm_color_button_normal, dm_color_button_border_enabled );
				}
			}

			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMComboBoxSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void EditPaint( HWND hWnd, HDC hDC, COLORREF border_color )
{
	LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );

	RECT window_rc;
	_GetWindowRect( hWnd, &window_rc );

	window_rc.right -= window_rc.left;
	window_rc.bottom -= window_rc.top;
	window_rc.top = 0;
	window_rc.left = 0;

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	HBITMAP hbm = _CreateCompatibleBitmap( hDC, window_rc.right, window_rc.bottom );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
	_DeleteObject( ohbm );
	_DeleteObject( hbm );

	HBRUSH color = _CreateSolidBrush( dm_color_edit_background );
	_FillRect( hdcMem, &window_rc, color );
	_DeleteObject( color );

	if ( style & WS_BORDER )
	{
		HPEN hPen = _CreatePen( PS_SOLID, 1, border_color );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );
		_Rectangle( hdcMem, 0, 0, window_rc.right, window_rc.bottom );
		_DeleteObject( hPen );
	}

	_BitBlt( hDC, 0, 0, window_rc.right, window_rc.bottom, hdcMem, 0, 0, SRCCOPY );

	_DeleteDC( hdcMem );
}

LRESULT CALLBACK DMIPSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			// Only update if it's visible.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == FALSE )
				{
					state |= 0x08;	// Was enabled
				}
				else
				{
					state |= 0x10;	// Was disabled.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );

				_InvalidateRect( hWnd, NULL, TRUE );	// Updates the dots.

				_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
			}

			// Don't return here.
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			_SetTextColor( ( HDC )wParam, dm_color_window_text );
			_SetBkMode( ( HDC )wParam, OPAQUE );
			_SetBkColor( ( HDC )wParam, dm_color_window_background );

			return ( LRESULT )g_hBrush_window_background;
		}
		break;

		case WM_CTLCOLOREDIT:
		{
			_SetTextColor( ( HDC )wParam, dm_color_window_text );
			_SetBkMode( ( HDC )wParam, OPAQUE );
			_SetBkColor( ( HDC )wParam, dm_color_edit_background );

			return ( LRESULT )g_hBrush_edit_background;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			COLORREF background_color;

			HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
			HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
			if ( _IsWindowEnabled( hWnd ) == TRUE )
			{
				background_color = dm_color_edit_background;
				_SetTextColor( hdcMem, dm_color_window_text );
			}
			else
			{
				background_color = dm_color_window_background;
				_SetTextColor( hdcMem, _GetSysColor( COLOR_GRAYTEXT ) );
			}
			// Transparent background for text.
			_SetBkMode( hdcMem, TRANSPARENT );

			HBRUSH color = _CreateSolidBrush( background_color );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			RECT rc;
			rc.left = client_rc.left;
			rc.top = client_rc.top + 1;
			rc.right = client_rc.right;
			rc.bottom = client_rc.bottom;

			RECT rc_child1_w, rc_child2_w;
			HWND child = _GetWindow( hWnd, GW_CHILD );
			_GetWindowRect( child, &rc_child1_w );
			child = _GetWindow( child, GW_HWNDNEXT );
			_GetWindowRect( child, &rc_child2_w );

			char direction;
			int space_width;
			if ( rc_child2_w.right < rc_child1_w.left )	// Traversing right to left.
			{
				direction = 1;
				space_width = ( rc_child1_w.left - rc_child2_w.right );
			}
			else	// Traversing left ot right.
			{
				direction = 0;
				space_width = ( rc_child2_w.left - rc_child1_w.right );
			}

			child = _GetWindow( hWnd, GW_CHILD );
			for ( char i = 0; i < 3 && child != NULL; ++i )
			{
				_GetWindowRect( child, &rc_child1_w );
				_MapWindowPoints( HWND_DESKTOP, hWnd, ( POINT * )&rc_child1_w, 2 );

				if ( direction == 1 )
				{
					rc.left = rc_child1_w.left - space_width;
					rc.right = rc_child1_w.left;
				}
				else
				{
					rc.left = rc_child1_w.right;
					rc.right = rc_child1_w.right + space_width;
				}

				_DrawTextW( hdcMem, L".", 1, &rc, DT_SINGLELINE | DT_CENTER );

				child = _GetWindow( child, GW_HWNDNEXT );
			}

			_SelectObject( hdcMem, ohf );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			if ( state == 0 )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );
			}

			return 0;
		}
		break;

		case WM_PARENTNOTIFY:
		{
			switch ( wParam )
			{
				case WM_KILLFOCUS:
				{
					HWND focused_hWnd = ( HWND )lParam;
					if ( _GetParent( focused_hWnd ) != hWnd )
					{
						_BufferedPaintStopAllAnimations( hWnd );

						DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

						// This condition handles the case where the control is focused and we switch to a different tab.
						// If we switch back, the previously focused control would have animated. We don't want that to happen.
						if ( _IsWindowVisible( hWnd ) != FALSE )
						{
							state |= 0x20;	// Was focused.
						}

						_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );

						//_RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASENOW );
						_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
					}
				}
				break;

				case WM_SETFOCUS:
				{
					HWND focused_hWnd = ( HWND )lParam;
					if ( _GetParent( focused_hWnd ) != hWnd )
					{
						_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x40 ) );

						//_RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASENOW );
						_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
					}
				}
				break;

				case WM_MOUSELEAVE:
				{
					_SendMessageW( hWnd, WM_MOUSELEAVE, 0, 0 );
				}
				break;

				case WM_MOUSEMOVE:
				{
					DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

					// Is it currently hovered?
					if ( !( state & 0x04 ) )
					{
						state |= 0x04;	// Set the hovered state.

						// We're going to update.
						_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );

						//_RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASENOW );
						_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
					}
				}
				break;
			}
			break;
		}
		break;

		//case WM_NCMOUSELEAVE:
		case WM_MOUSELEAVE:
		{
			POINT pt;
			_GetCursorPos( &pt );
			RECT rc;
			_GetClientRect( hWnd, &rc );
			_MapWindowPoints( HWND_DESKTOP, hWnd, &pt, 1 );

			if ( pt.x <= rc.left || pt.x >= rc.right || pt.y <= rc.top || pt.y >= rc.bottom )
			{
				_BufferedPaintStopAllAnimations( hWnd );

				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				state &= ~0x04;		// Unset the hovered state.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );

				//_RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASENOW );
				_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
			}
		}
		break;

		//case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		{
			int xPos = GET_X_LPARAM( lParam ); 
			int yPos = GET_Y_LPARAM( lParam );

			RECT rc;
			_GetClientRect( hWnd, &rc );

			if ( xPos > rc.left && xPos < rc.right && yPos > rc.top && yPos < rc.bottom )
			{
				TRACKMOUSEEVENT tmi;
				tmi.cbSize = sizeof( TRACKMOUSEEVENT );
				tmi.dwFlags = TME_LEAVE;// | TME_NONCLIENT;
				tmi.hwndTrack = hWnd;
				_TrackMouseEvent( &tmi );

				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				// Is it currently hovered?
				if ( !( state & 0x04 ) )
				{
					state |= 0x04;	// Set the hovered state.

					_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );

					//_RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASENOW );
					_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
				}
			}
		}
		break;

		case WM_NCCALCSIZE:
		{
			if ( wParam == FALSE )
			{
				RECT *pRect = ( RECT * )lParam;
				pRect->top += 2;
				pRect->left += 2;
				pRect->bottom -= 2;
				pRect->right -= 2;
			}
			else// if ( wParam == TRUE )
			{
				NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
				nccsp->rgrc[ 0 ].top += 2;
				nccsp->rgrc[ 0 ].left += 2;
				nccsp->rgrc[ 0 ].bottom -= 2;
				nccsp->rgrc[ 0 ].right -= 2;
			}

			return 0;
		}
		break;

		case WM_ERASEBKGND:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x40 ) );
		}
		break;

		case WM_NCPAINT:
		{
			HDC hDC = _GetWindowDC( hWnd );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x40 ) )
			{
				state &= ~0x40;

				bool color_state1 =	( state & 0x01 ? true : false );	// hovered
				bool color_state2 =	( state & 0x02 ? true : false );	// focused
				bool is_hovered =	( state & 0x04 ? true : false );
				bool was_enabled =	( state & 0x08 ? true : false );
				bool was_disabled =	( state & 0x10 ? true : false );
				bool was_focused =	( state & 0x20 ? true : false );

				state &= ~0x08;
				state &= ~0x10;
				state &= ~0x20;

				bool is_focused = false;
				HWND child = _GetWindow( hWnd, GW_CHILD );
				for ( char i = 0; i < 4 && child != NULL; ++i )
				{
					if ( child == _GetFocus() )
					{
						is_focused = true;

						break;
					}

					child = _GetWindow( child, GW_HWNDNEXT );
				}

				COLORREF border_color1;
				COLORREF border_color2;

				if ( _IsWindowEnabled( hWnd ) != FALSE )
				{
					if ( is_hovered )
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;
							border_color2 = dm_color_edit_border_hover;

							state |= 0x01;		// Turn on the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else if ( is_focused )
						{
							if ( !color_state2 )
							{
								if ( color_state1 )
								{
									border_color1 = dm_color_edit_border_hover;
								}
								else
								{
									border_color1 = dm_color_edit_border_enabled;
								}

								state |= 0x02;	// Turn on focused color state.
							}
							else
							{
								border_color1 = dm_color_focus_rectangle;
							}

							border_color2 = dm_color_focus_rectangle;
						}
						else if ( !color_state1 )
						{
							border_color1 = dm_color_edit_border_enabled;
							border_color2 = dm_color_edit_border_hover;

							state |= 0x01;		// Turn on the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else
						{
							border_color1 = dm_color_edit_border_hover;
							border_color2 = border_color1;

							state &= ~0x02;		// Turn off the focused color state.
						}
					}
					else
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;
							border_color2 = dm_color_edit_border_enabled;

							state &= ~0x01;		// Turn off the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else if ( is_focused )
						{
							if ( !color_state2 )
							{
								border_color1 = dm_color_edit_border_enabled;
							}
							else
							{
								border_color1 = dm_color_focus_rectangle;
							}

							border_color2 = dm_color_focus_rectangle;

							state &= ~0x01;		// Turn off the hovered color state.
							state |= 0x02;		// Turn on focused color state.
						}
						else if ( color_state1 )
						{
							border_color1 = dm_color_edit_border_hover;
							border_color2 = dm_color_edit_border_enabled;

							state &= ~0x01;		// Turn off the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else
						{
							border_color1 = dm_color_edit_border_enabled;
							border_color2 = border_color1;

							state &= ~0x02;		// Turn off the focused color state.
						}
					}
				}
				else
				{
					state &= ~0x01;		// Turn off the hovered color state.
					state &= ~0x02;		// Turn off the focused color state.

					border_color1 = dm_color_edit_border_disabled;
					border_color2 = dm_color_edit_border_disabled;
				}

				if ( was_enabled )
				{
					border_color1 = dm_color_edit_border_enabled;
				}
				else if ( was_disabled )
				{
					border_color1 = dm_color_edit_border_disabled;
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				//if ( was_enabled || was_disabled || border_color1 != border_color2 )
				{
					BP_ANIMATIONPARAMS animParams;
					_memzero( &animParams, sizeof( animParams ) );
					animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
					animParams.style = BPAS_LINEAR;

					if ( border_color1 != border_color2 )
					{
						animParams.dwDuration = BUTTON_ANIMATION_DURATION / 2;
					}

					BP_PAINTPARAMS paintParams;
					_memzero( &paintParams, sizeof( BP_PAINTPARAMS ) );
					paintParams.cbSize = sizeof( BP_PAINTPARAMS );
					paintParams.dwFlags = BPPF_ERASE | BPPF_NONCLIENT;

					RECT window_rc;
					_GetWindowRect( hWnd, &window_rc );
					window_rc.right -= window_rc.left;
					window_rc.bottom -= window_rc.top;
					window_rc.top = 0;
					window_rc.left = 0;

					RECT rc_exclude;
					rc_exclude.top = 2;
					rc_exclude.left = 2;
					rc_exclude.right = window_rc.right - 2;
					rc_exclude.bottom = window_rc.bottom - 2;

					paintParams.prcExclude = &rc_exclude;

					HDC hdcFrom, hdcTo;
					HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &window_rc, BPBF_COMPATIBLEBITMAP, &paintParams, &animParams, &hdcFrom, &hdcTo );
					if ( hbpAnimation )
					{
						if ( hdcFrom )
						{
							EditPaint( hWnd, hdcFrom, border_color1 );
						}

						if ( hdcTo )
						{
							EditPaint( hWnd, hdcTo, border_color2 );
						}

						_EndBufferedAnimation( hbpAnimation, TRUE );
					}
					else
					{
						EditPaint( hWnd, hDC, border_color1 );
					}
				}
				/*else
				{
					EditPaint( hWnd, hDC, border_color1 );
				}*/
			}

			_ReleaseDC( hWnd, hDC );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMIPSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMListViewCustomDrawSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case NM_CUSTOMDRAW:
				{
					NMCUSTOMDRAW *nmcd = ( NMCUSTOMDRAW * )lParam;
					switch ( nmcd->dwDrawStage )
					{
						case CDDS_PREPAINT:
						{
							return CDRF_NOTIFYITEMDRAW;
						}
						break;

						case CDDS_ITEMPREPAINT:
						{
							_SetTextColor( nmcd->hdc, dm_color_window_text );

							return CDRF_DODEFAULT;
						}
						break;
					}
				}
				break;
			}
		}
		break;

		case WM_PAINT:
		{
			HWND hWnd_parent = _GetParent( hWnd );
			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			RECT rc_header;

			HWND hWnd_header = ( HWND )_SendMessageW( hWnd, LVM_GETHEADER, 0, 0 );
			int header_count = ( int )_SendMessageW( hWnd_header, HDM_GETITEMCOUNT, 0, 0 );

			_GetClientRect( hWnd_header, &rc_header );
			int client_offset = rc_header.bottom;

			unsigned int width = client_rc.right - client_rc.left;
			unsigned int height = client_rc.bottom - client_rc.top;

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, width, height );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			_SendMessageW( hWnd, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT /*| PRF_NONCLIENT*/ ) );

			SCROLLINFO si;
			_memzero( &si, sizeof( SCROLLINFO ) );
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			_GetScrollInfo( hWnd, SB_HORZ, &si );

			// No items in the list.
			if ( si.nPage < width )
			{
				si.nPage = width;
			}

			HPEN hPen = _CreatePen( PS_SOLID, _SCALE_DM_( 1 ), dm_color_listview_grid );
			HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
			_DeleteObject( old_color );
			HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
			_DeleteObject( old_brush );

			for ( int i = 0; i < header_count; ++i )
			{
				_SendMessageW( hWnd_header, HDM_GETITEMRECT, i, ( LPARAM )&rc_header );

				if ( rc_header.right < si.nPos )
				{
					continue;
				}
				else if ( rc_header.left > ( int )( si.nPos + si.nPage ) )
				{
					break;
				}

				_MoveToEx( hdcMem, rc_header.right - si.nPos, ps.rcPaint.top, NULL );
				_LineTo( hdcMem, rc_header.right - si.nPos, ps.rcPaint.bottom );
			}

			int row_offset = rc_header.bottom - 1;//_SCALE_DM_( g_default_row_height ) + _SCALE_DM_( 3 );

			while ( row_offset <= client_rc.bottom )
			{
				_MoveToEx( hdcMem, 0, row_offset, NULL );
				_LineTo( hdcMem, client_rc.right, row_offset );

				row_offset += _SCALE_DM_( g_default_row_height );
			}

			_DeleteObject( hPen );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top + client_offset, client_rc.right, client_rc.bottom, hdcMem, 0, client_offset, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );

			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMListViewCustomDrawSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMTreeViewSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_DPICHANGED_AFTERPARENT:
		{
			DWORD style = ( DWORD )_GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( style & TVS_CHECKBOXES )
			{
				HWND hWnd_parent = _GetParent( hWnd );

				UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
				
				UINT last_dpi_dark_mode = ( UINT )_GetPropW( hWnd, L"DPI" );

				if ( current_dpi_dark_mode != 0 && last_dpi_dark_mode != 0 && current_dpi_dark_mode != last_dpi_dark_mode )
				{
					_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

					HIMAGELIST hIL = ( HIMAGELIST )_SendMessageW( hWnd, TVM_GETIMAGELIST, TVSIL_STATE, 0 );
					if ( hIL != NULL )
					{
						_ImageList_Destroy( hIL );
					}

					hIL = UpdateButtonGlyphs( hWnd, NULL );
					if ( hIL != NULL )
					{
						// The first image is empty, the next two are unchecked and checked.
						// The treeview's unchecked/checked state value (bits 12 through 15) switches between 0x1000 and 0x2000.
						_ImageList_SetImageCount( hIL, 3 );

						_SendMessageW( hWnd, TVM_SETIMAGELIST, TVSIL_STATE, ( LPARAM )hIL );
					}
				}

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_DESTROY:
		{
			DWORD style = ( DWORD )_GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( style & TVS_CHECKBOXES )
			{
				HIMAGELIST hIL = ( HIMAGELIST )_SendMessageW( hWnd, TVM_GETIMAGELIST, TVSIL_STATE, 0 );
				if ( hIL != NULL )
				{
					_ImageList_Destroy( hIL );
				}
			}
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMTreeViewSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMStatusBarSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_MOUSEHOVER:
		{
			if ( g_sb_is_tracking == TRUE )
			{
				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;

				tti.lpszText = status_bar_tooltip_text;

				_SendMessageW( g_hWnd_status_bar_tooltip, TTM_SETTOOLINFO, 0, ( LPARAM )&tti );
				_SendMessageW( g_hWnd_status_bar_tooltip, TTM_TRACKACTIVATE, TRUE, ( LPARAM )&tti );
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			int tracking_x = GET_X_LPARAM( lParam );
			int tracking_y = GET_Y_LPARAM( lParam );

			if ( tracking_x != g_sb_tracking_x || tracking_y != g_sb_tracking_y )
			{
				g_sb_is_tracking = FALSE;

				g_sb_tracking_x = tracking_x;
				g_sb_tracking_y = tracking_y;

				RECT rc;
				// Work backwards because the panels on the right will overlap those that are to its left.
				for ( char i = 3; i >= 0; --i )
				{
					_SendMessageW( hWnd, SB_GETRECT, i, ( LPARAM )&rc );

					if ( g_sb_tracking_x >= rc.left && g_sb_tracking_x <= rc.right &&
						 g_sb_tracking_y >= rc.top && g_sb_tracking_y <= rc.bottom )
					{
						rc.right -= 2;	// We move the text left by 2px when drawing it. So the bounding width is essentially 2px less.

						HDC hDC = _GetDC( hWnd );
						HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
						HFONT ohf = ( HFONT )_SelectObject( hDC, hFont );

						RECT rc_text;
						rc_text.left = rc.left;
						rc_text.right = rc.right;
						rc_text.top = rc.top;
						rc_text.bottom = rc.bottom;

						//_SendMessageW( g_hWnd_status, SB_GETTIPTEXT, MAKEWPARAM( i, sizeof( wchar_t ) * 64 ), ( LPARAM )status_bar_tooltip_text );
						wchar_t *buf;
						int len = ( int )_SendMessageW( hWnd, SB_GETTEXTLENGTH, i, 0 ) + 1;	// Include the NULL character.
						if ( len > 64 )
						{
							buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
						}
						else
						{
							buf = status_bar_tooltip_text;
						}

						if ( buf != NULL )
						{
							DWORD ret = ( DWORD )_SendMessageW( g_hWnd_status, SB_GETTEXT, i, ( LPARAM )buf );

							_DrawTextW( hDC, buf, LOWORD( ret ), &rc_text, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_CALCRECT );

							if ( buf != status_bar_tooltip_text )
							{
								GlobalFree( buf );
							}
						}

						_SelectObject( hDC, ohf );
						_ReleaseDC( hWnd, hDC );

						if ( rc_text.right > rc.right )
						{
							TOOLINFO tti;
							_memzero( &tti, sizeof( TOOLINFO ) );
							tti.cbSize = sizeof( TOOLINFO );
							tti.hwnd = hWnd;
							tti.uId = ( UINT_PTR )hWnd;
							_SendMessageW( g_hWnd_status_bar_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );

							TRACKMOUSEEVENT tmi;
							tmi.cbSize = sizeof( TRACKMOUSEEVENT );
							tmi.dwFlags = TME_HOVER;
							tmi.hwndTrack = hWnd;
							tmi.dwHoverTime = HOVER_DEFAULT;
							g_sb_is_tracking = _TrackMouseEvent( &tmi );
						}

						break;
					}
				}
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( dm_color_window_background );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			HPEN line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
			HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
			_DeleteObject( old_color );

			_MoveToEx( hdcMem, client_rc.left, client_rc.top, NULL );
			_LineTo( hdcMem, client_rc.right, client_rc.top );

			RECT rc;
			for ( char i = 0; i < 4; ++i )
			{
				_SendMessageW( hWnd, SB_GETRECT, i, ( LPARAM )&rc );

				if ( i < 3 )
				{
					_MoveToEx( hdcMem, rc.right, rc.top, NULL );
					_LineTo( hdcMem, rc.right, rc.top );
					_LineTo( hdcMem, rc.right, rc.bottom - rc.top );
				}

				rc.left += 2;

				HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
				HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
				_SetTextColor( hdcMem, dm_color_window_text );
				// Transparent background for text.
				_SetBkMode( hdcMem, TRANSPARENT );

				wchar_t *buf;
				wchar_t tbuf[ 64 ];
				int len = ( int )_SendMessageW( hWnd, SB_GETTEXTLENGTH, i, 0 ) + 1;	// Include the NULL character.
				if ( len > 64 )
				{
					buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
				}
				else
				{
					buf = tbuf;
				}

				if ( buf != NULL )
				{
					DWORD ret = ( DWORD )_SendMessageW( hWnd, SB_GETTEXT, i, ( LPARAM )buf );

					_DrawTextW( hdcMem, buf, LOWORD( ret ), &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );

					if ( buf != tbuf )
					{
						GlobalFree( buf );
					}
				}

				_SelectObject( hdcMem, ohf );
			}

			_DeleteObject( line_color );

			if ( hTheme_status == NULL )
			{
				hTheme_status = _OpenThemeData( hWnd, L"Status" );
			}

			if ( hTheme_status != NULL )
			{
				SIZE grip_glyph_size;
				_GetThemePartSize( hTheme_status, NULL, SP_GRIPPER, 0, NULL, TS_DRAW, &grip_glyph_size );

				rc.left = client_rc.right - grip_glyph_size.cx;
				rc.top = client_rc.bottom - grip_glyph_size.cy;
				rc.right = client_rc.right;
				rc.bottom = client_rc.bottom;
				_DrawThemeBackground( hTheme_status, hdcMem, SP_GRIPPER, 0, &rc, 0 );
			}

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMStatusBarSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMControlColorSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORLISTBOX:
		{
			_SetTextColor( ( HDC )wParam, dm_color_window_text );
			_SetBkMode( ( HDC )wParam, OPAQUE );
			_SetBkColor( ( HDC )wParam, dm_color_window_background );

			return ( LRESULT )g_hBrush_window_background;
		}
		break;

		case WM_CTLCOLOREDIT:
		{
			_SetTextColor( ( HDC )wParam, dm_color_window_text );
			_SetBkMode( ( HDC )wParam, OPAQUE );	// TRANSPARENT causes issues in a multiline edit control.

			// WM_CTLCOLOREDIT gets called for combo boxes if CB_SETCURSEL is called.
			// We don't want them to use the edit control's colors if it's not a dropdown.
			wchar_t buf[ 64 ];
			HWND hWnd_parent = ( HWND )lParam;
			if ( hWnd_parent != NULL )
			{
				int ret = _GetClassNameW( hWnd_parent, buf, 64 );
				if ( ret == 8 && _StrCmpNW( buf, L"ComboBox", 8 ) == 0 )
				{
					LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
					if ( ( style & 0x0000000F ) != CBS_DROPDOWN )
					{
						/*DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

						//bool color_state1 = ( state & 0x01 ? true : false );	// hovered
						//bool color_state2 = ( state & 0x02 ? true : false );	// down
						bool is_hovered =	( state & 0x04 ? true : false );
						bool is_down =		( state & 0x08 ? true : false );

						COLORREF background_color;
						if ( is_down )
						{
							background_color = dm_color_button_pressed;
						}
						else if ( is_hovered )
						{
							background_color = dm_color_button_hover;
						}
						else
						{
							background_color = dm_color_button_normal;
						}
						_SetBkColor( ( HDC )wParam, background_color );*/

						_SetBkColor( ( HDC )wParam, dm_color_button_normal );

						return ( LRESULT )_GetStockObject( NULL_BRUSH );
					}
				}
			}

			_SetBkColor( ( HDC )wParam, dm_color_edit_background );

			return ( LRESULT )g_hBrush_edit_background;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			if ( dis->CtlType == ODT_COMBOBOX && !( dis->itemState & ODS_COMBOBOXEDIT ) || dis->CtlType == ODT_LISTBOX )
			{
				_FillRect( dis->hDC, &dis->rcItem, g_hBrush_window_background );

				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					HBRUSH color = _CreateSolidBrush( dm_color_list_highlight );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				_SetTextColor( dis->hDC, dm_color_window_text );

				// Transparent background for text.
				_SetBkMode( dis->hDC, TRANSPARENT );

				wchar_t *buf;
				wchar_t tbuf[ 128 ];
				int len = ( int )_SendMessageW( hWnd, ( dis->CtlType == ODT_COMBOBOX ? CB_GETLBTEXTLEN : LB_GETTEXTLEN ), dis->itemID, 0 ) + 1;	// Include the NULL character.
				if ( len > 128 )
				{
					buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * len );
				}
				else
				{
					buf = tbuf;
				}

				if ( buf != NULL )
				{
					int len = ( int )_SendMessageW( dis->hwndItem, ( dis->CtlType == ODT_COMBOBOX ? CB_GETLBTEXT : LB_GETTEXT ), dis->itemID, ( LPARAM )buf );

					RECT rc;
					rc.left = dis->rcItem.left + ( dis->CtlType == ODT_COMBOBOX ? 4 : 2 );
					rc.top = dis->rcItem.top;
					rc.right = dis->rcItem.right;
					rc.bottom = dis->rcItem.bottom;
					_DrawTextW( dis->hDC, buf, len, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );

					if ( buf != tbuf )
					{
						GlobalFree( buf );
					}
				}
			}
			else
			{
				return _DefSubclassProc( hWnd, msg, wParam, lParam );
			}

			return TRUE;
		}
		break;

		case WM_UAHDRAWMENU:
		{
			UAHMENU *pUDM = ( UAHMENU * )lParam;

			MENUBARINFO mbi;
			mbi.cbSize = sizeof( MENUBARINFO );
			_GetMenuBarInfo( hWnd, OBJID_MENU, 0, &mbi );

			RECT rc;
			_GetWindowRect( hWnd, &rc );

			_OffsetRect( &mbi.rcBar, -rc.left, -rc.top );

			_FillRect( pUDM->hDC, &mbi.rcBar, g_hBrush_window_background );

			return true;
		}
		break;

		case WM_UAHDRAWMENUITEM:
		{
			UAHDRAWMENUITEM *pUDMI = ( UAHDRAWMENUITEM * )lParam;

			wchar_t buf[ 128 ];

			MENUITEMINFOW mii;
			mii.cbSize = sizeof( MENUITEMINFOW );
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = buf;
			mii.cch = 128;
			_GetMenuItemInfoW( pUDMI->um.hMenu, pUDMI->umi.iPosition, TRUE, &mii );

			DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

			HBRUSH color;

			int iTextStateID = 0;
			int iBackgroundStateID = 0;

			if ( ( pUDMI->dis.itemState & ODS_HOTLIGHT ) || ( pUDMI->dis.itemState & ODS_SELECTED ) )
			{
				color = _CreateSolidBrush( dm_color_menu_hover );

				iTextStateID = MPI_HOT;
				iBackgroundStateID = MPI_HOT;
			}
			else if ( ( pUDMI->dis.itemState & ODS_GRAYED ) || ( pUDMI->dis.itemState & ODS_DISABLED ) )
			{
				color = _CreateSolidBrush( dm_color_window_background );

				iTextStateID = MPI_DISABLED;
				iBackgroundStateID = MPI_DISABLED;
			}
			else// if ( ( pUDMI->dis.itemState & ODS_INACTIVE ) || ( pUDMI->dis.itemState & ODS_DEFAULT ) )
			{
				color = _CreateSolidBrush( dm_color_window_background );

				iTextStateID = MPI_NORMAL;
				iBackgroundStateID = MPI_NORMAL;
			}

			if ( pUDMI->dis.itemState & ODS_NOACCEL )
			{
				dwFlags |= DT_HIDEPREFIX;
			}

			if ( g_hTheme_menu == NULL )
			{
				g_hTheme_menu = _OpenThemeData( hWnd, L"Menu" );
			}

			_FillRect( pUDMI->um.hDC, &pUDMI->dis.rcItem, color );
			_DeleteObject( color );

			if ( g_hTheme_menu != NULL )
			{
				DTTOPTS opts;
				opts.dwSize = sizeof( opts );
				opts.dwFlags = DTT_TEXTCOLOR;
				opts.crText = iTextStateID != MPI_DISABLED ? dm_color_window_text : ( COLORREF )_GetSysColor( COLOR_GRAYTEXT );
				_DrawThemeTextEx( g_hTheme_menu, pUDMI->um.hDC, MENU_BARITEM, MBI_NORMAL, buf, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts );
			}

			return true;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMControlColorSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMMsgBoxColorSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		{
			_SetTextColor( ( HDC )wParam, dm_color_window_text );
			_SetBkMode( ( HDC )wParam, OPAQUE );
			_SetBkColor( ( HDC )wParam, dm_color_edit_background );

			return ( LRESULT )g_hBrush_edit_background;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMMsgBoxColorSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMTabControlSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_LBUTTONDOWN:
		{
			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_KEYDOWN:
		{
			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			_FillRect( hdcMem, &client_rc, g_hBrush_window_background );

			// Get the tab item's height. (bottom)
			RECT rc_tab;
			_SendMessageW( hWnd, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			// Draw our tab border.
			RECT rc_pane;
			rc_pane.left = 0;
			rc_pane.top = rc_tab.bottom;
			rc_pane.bottom = client_rc.bottom;
			rc_pane.right = client_rc.right;

			HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_window_border );
			HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
			_DeleteObject( old_color );
			HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
			_DeleteObject( old_brush );
			_Rectangle( hdcMem, rc_pane.left, rc_pane.top, rc_pane.right, rc_pane.bottom );
			_DeleteObject( hPen );

			HFONT hFont = ( HFONT )_SendMessageW( hWnd, WM_GETFONT, 0, 0 );
			HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
			if ( _IsWindowEnabled( hWnd ) == TRUE )
			{
				_SetTextColor( hdcMem, dm_color_window_text );
			}
			else
			{
				_SetTextColor( hdcMem, _GetSysColor( COLOR_GRAYTEXT ) );
			}
			// Transparent background for text.
			_SetBkMode( hdcMem, TRANSPARENT );

			HPEN line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
			old_color = ( HPEN )_SelectObject( hdcMem, line_color );
			_DeleteObject( old_color );


			wchar_t tab_text[ 64 ];
			TCITEM tci;
			_memzero( &tci, sizeof( TCITEM ) );
			tci.mask = TCIF_TEXT;
			tci.pszText = tab_text;
			tci.cchTextMax = 64;

			int sel_index = ( int )_SendMessageW( hWnd, TCM_GETCURSEL, 0, 0 );	// Get the selected tab

			TCHITTESTINFO tchti;
			_GetCursorPos( &tchti.pt );
			_ScreenToClient( hWnd, &tchti.pt );
			int hover_index = ( int )_SendMessageW( hWnd, TCM_HITTEST, 0, ( LPARAM )&tchti );

			// Get the bounding rect for each tab item.
			int tab_count = ( int )_SendMessageW( hWnd, TCM_GETITEMCOUNT, 0, 0 );
			for ( int i = 0; i < tab_count; ++i )
			{
				// Exclude the selected tab. We draw it last so it can clip the non-selected tabs.
				if ( i != sel_index )
				{
					_SendMessageW( hWnd, TCM_GETITEMRECT, i, ( LPARAM )&rc_tab );

					if ( rc_tab.left >= 0 )
					{
						if ( i == hover_index )
						{
							HBRUSH color = _CreateSolidBrush( dm_color_button_hover );
							_FillRect( hdcMem, &rc_tab, color );
							_DeleteObject( color );
						}
						else
						{
							_FillRect( hdcMem, &rc_tab, g_hBrush_window_background );
						}

						_MoveToEx( hdcMem, rc_tab.left, rc_tab.bottom, NULL );
						_LineTo( hdcMem, rc_tab.left, rc_tab.top );
						_LineTo( hdcMem, rc_tab.right, rc_tab.top );
						_LineTo( hdcMem, rc_tab.right, rc_tab.bottom );

						// Offset the text position.
						++rc_tab.top;

						_SendMessageW( hWnd, TCM_GETITEM, i, ( LPARAM )&tci );	// Get the tab's information.
						_DrawTextW( hdcMem, tci.pszText, -1, &rc_tab, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );
					}
				}
			}

			_SendMessageW( hWnd, TCM_GETITEMRECT, sel_index, ( LPARAM )&rc_tab );

			// Only show the tab if it's completely visible.
			if ( rc_tab.left > 0 )
			{
				// Position of our text (offset up).
				RECT rc_text;
				rc_text.left = rc_tab.left;
				rc_text.right = rc_tab.right;
				rc_text.top  = rc_tab.top - 1;
				rc_text.bottom = rc_tab.bottom - 2;

				// Enlarge the selected tab's area.
				rc_tab.left -= 2;
				rc_tab.top -= 2;
				++rc_tab.bottom;
				rc_tab.right += 2;

				_FillRect( hdcMem, &rc_tab, g_hBrush_window_background );

				--rc_tab.bottom;

				_MoveToEx( hdcMem, rc_tab.left, rc_tab.bottom, NULL );
				_LineTo( hdcMem, rc_tab.left, rc_tab.top );
				_LineTo( hdcMem, rc_tab.right, rc_tab.top );
				_LineTo( hdcMem, rc_tab.right, rc_tab.bottom );

				_SendMessageW( hWnd, TCM_GETITEM, sel_index, ( LPARAM )&tci );	// Get the tab's information.
				_DrawTextW( hdcMem, tci.pszText, -1, &rc_text, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS );

				DWORD ui_state = ( DWORD )_SendMessageW( hWnd, WM_QUERYUISTATE, 0, 0 );
				if ( !( ui_state & UISF_HIDEFOCUS ) && hWnd == _GetFocus() )
				{
					rc_tab.left += 3;
					rc_tab.right -= 2;
					rc_tab.top += 3;
					rc_tab.bottom -= 1;
					//_DrawFocusRect( hdcMem, &rc_tab );	// This doesn't look consistent.

					LOGBRUSH lb;
					lb.lbColor = dm_color_focus_rectangle;
					lb.lbStyle = PS_SOLID;
					hPen = _ExtCreatePen( PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, NULL );
					old_color = ( HPEN )_SelectObject( hdcMem, hPen );
					_DeleteObject( old_color );
					old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
					_DeleteObject( old_brush );
					_Rectangle( hdcMem, rc_tab.left, rc_tab.top, rc_tab.right, rc_tab.bottom );
					_DeleteObject( hPen );
				}
			}

			_DeleteObject( line_color );

			_SelectObject( hdcMem, ohf );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			// Delete our back buffer.
			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMTabControlSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void ToolbarPaint( HWND hWnd, HDC hDC, int index1, COLORREF button1_color, int index2, COLORREF button2_color )
{
	HWND hWnd_parent = _GetParent( hWnd );
	UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	RECT client_rc;
	_GetClientRect( hWnd, &client_rc );

	HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
	_DeleteObject( ohbm );
	_DeleteObject( hbm );

	_FillRect( hdcMem, &client_rc, g_hBrush_window_background );

	// Figure out where the lines are being drawn (5px above each row) and cover them up with our own lines.
	int last_index = ( int )_SendMessageW( hWnd, TB_BUTTONCOUNT, 0, 0 ) - 1;
	if ( last_index >= 0 )
	{
		RECT rc_row;

		HIMAGELIST hIL = ( HIMAGELIST )_SendMessageW( hWnd, TB_GETIMAGELIST, 0, 0 );
		int il_offset = 0;

		RECT last_rc_row;
		last_rc_row.left = 0;
		last_rc_row.top = 0;
		last_rc_row.right = 0;
		last_rc_row.bottom = 0;

		for ( int i = 0; i <= last_index; ++i )
		{
			TBBUTTON tbb;
			_SendMessageW( hWnd, TB_GETBUTTON, i, ( LPARAM )&tbb );
			_SendMessageW( hWnd, TB_GETITEMRECT, i, ( LPARAM )&rc_row );

			if ( tbb.fsStyle & BTNS_SEP )
			{
				++il_offset;

				HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
				_DeleteObject( old_color );
				HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
				_DeleteObject( old_brush );

				// Keep the separator at the end of the previous row.
				if ( rc_row.left == 0 )
				{
					rc_row.right = last_rc_row.right + ( rc_row.right - rc_row.left );
					rc_row.left = last_rc_row.right;
					rc_row.top = last_rc_row.top;
					rc_row.bottom = last_rc_row.bottom;
				}

				int offset = rc_row.left + ( ( rc_row.right - rc_row.left ) / 2 );
				_MoveToEx( hdcMem, offset, rc_row.top + 2, NULL );
				_LineTo( hdcMem, offset, rc_row.bottom - 1 );

				_DeleteObject( hPen );
			}
			else if ( tbb.fsState & TBSTATE_ENABLED )
			{
				if ( i == index1 || i == index2 )
				{
					RECT rc_hover;
					rc_hover.left = rc_row.left;
					rc_hover.right = rc_row.right;
					rc_hover.top = rc_row.top + 1;
					rc_hover.bottom = rc_row.bottom;

					COLORREF button_color = ( i == index1 ? button1_color : ( i == index2 ? button2_color : dm_color_button_normal ) );

					HBRUSH color = _CreateSolidBrush( button_color );
					_FillRect( hdcMem, &rc_hover, color );
					_DeleteObject( color );

					HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_button_border_enabled );
					HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
					_DeleteObject( old_color );
					HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
					_DeleteObject( old_brush );
					_Rectangle( hdcMem, rc_hover.left, rc_hover.top, rc_hover.right, rc_hover.bottom );
					_DeleteObject( hPen );
				}

				if ( tbb.fsState & TBSTATE_PRESSED )
				{
					++rc_row.left;
				}

				_ImageList_Draw( hIL, i - il_offset, hdcMem, rc_row.left + 3, rc_row.top + 3, 0 );
			}
			else
			{
				int icon_height_wdith = _SCALE_DM_( 24 );

				HDC hdcMem2 = _CreateCompatibleDC( hDC );
				HBITMAP hbm2 = _CreateCompatibleBitmap( hDC, icon_height_wdith, icon_height_wdith );
				HBITMAP ohbm2 = ( HBITMAP )_SelectObject( hdcMem2, hbm2 );
				_DeleteObject( ohbm2 );
				_DeleteObject( hbm2 );

				HDC hdcMem3 = _CreateCompatibleDC( hDC );
				HBITMAP hbm3 = _CreateCompatibleBitmap( hDC, icon_height_wdith, icon_height_wdith );
				HBITMAP ohbm3 = ( HBITMAP )_SelectObject( hdcMem3, hbm3 );
				_DeleteObject( ohbm3 );
				_DeleteObject( hbm3 );

				// Gray icon.

				RECT rc;
				rc.left = 0;
				rc.right = icon_height_wdith;
				rc.top = 0;
				rc.bottom = icon_height_wdith;

				// Set to white so we can invert it after the mask is drawn.
				HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xFF, 0xFF, 0xFF ) );
				_FillRect( hdcMem2, &rc, color );
				_DeleteObject( color );

				_ImageList_Draw( hIL, i - il_offset, hdcMem2, 0, 0, ILD_MASK );

				// The color of the icon.
				color = _CreateSolidBrush( _GetSysColor( COLOR_GRAYTEXT ) );
				_FillRect( hdcMem3, &rc, color );
				_DeleteObject( color );

				// The white pixels in hdcMem2 turn to black and the black mask will turn to white.
				_BitBlt( hdcMem2, 0, 0, icon_height_wdith, icon_height_wdith, hdcMem3, 0, 0, SRCERASE );

				// The black pixels in hdcMem2 will not show up in hdcMem.
				_BitBlt( hdcMem, rc_row.left + 3, rc_row.top + 3, icon_height_wdith, icon_height_wdith, hdcMem2, 0, 0, SRCPAINT );

				// Transparent icon.

				/*_ImageList_Draw( hIL, i - il_offset, hdcMem2, 0, 0, 0 );

				BLENDFUNCTION blend;
				blend.BlendOp = AC_SRC_OVER;
				blend.BlendFlags = 0;
				blend.SourceConstantAlpha = 70;
				blend.AlphaFormat = AC_SRC_OVER;

				_GdiAlphaBlend( hdcMem3, 0, 0, icon_height_wdith, icon_height_wdith, hdcMem2, 0, 0, icon_height_wdith, icon_height_wdith, blend );

				_BitBlt( hdcMem, rc_row.left + 3, rc_row.top + 3, icon_height_wdith, icon_height_wdith, hdcMem3, 0, 0, SRCPAINT );*/

				_DeleteDC( hdcMem2 );
				_DeleteDC( hdcMem3 );
			}

			last_rc_row.left = rc_row.left;
			last_rc_row.top = rc_row.top;
			last_rc_row.right = rc_row.right;
			last_rc_row.bottom = rc_row.bottom;
		}

		_SendMessageW( hWnd, TB_GETITEMRECT, last_index, ( LPARAM )&rc_row );

		int last_row_top = rc_row.top;
		rc_row.top -= 5;

		HPEN hPen = _CreatePen( PS_SOLID, 1, dm_color_toolbar_separator );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
		_DeleteObject( old_color );
		HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
		_DeleteObject( old_brush );

		int rows = ( int )_SendMessageW( hWnd, TB_GETROWS, 0, 0 );
		for ( int i = 0; i < rows && rc_row.top > 0; ++i )
		{
			RECT cover_line;
			cover_line.top = rc_row.top;
			cover_line.left = client_rc.left;
			cover_line.right = client_rc.right;
			cover_line.bottom = cover_line.top + 2;
			_FillRect( hdcMem, &cover_line, g_hBrush_window_background );

			_MoveToEx( hdcMem, client_rc.left, rc_row.top, NULL );
			_LineTo( hdcMem, client_rc.right, rc_row.top );

			// Work backwards since the first row's height doesn't match the other rows and we also don't want to display a line for empty rows.
			while ( last_index-- > 0 )
			{
				_SendMessageW( hWnd, TB_GETITEMRECT, last_index, ( LPARAM )&rc_row );

				if ( rc_row.top != last_row_top )
				{
					break;
				}
			}

			last_row_top = rc_row.top;
			rc_row.top -= 5;
		}

		_DeleteObject( hPen );
	}

	// Draw our memory buffer to the main device context.
	_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

	// Delete our back buffer.
	_DeleteDC( hdcMem );
}

LRESULT CALLBACK DMToolbarSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_MOUSEHOVER:
		{
			if ( g_tb_is_tracking == TRUE )
			{
				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;

				tti.lpszText = toolbar_tooltip_text;

				_SendMessageW( g_hWnd_toolbar_tooltip, TTM_SETTOOLINFO, 0, ( LPARAM )&tti );
				_SendMessageW( g_hWnd_toolbar_tooltip, TTM_TRACKACTIVATE, TRUE, ( LPARAM )&tti );
			}
		}
		break;

		//case TB_AUTOSIZE:
		case WM_WINDOWPOSCHANGING:
		case TB_SETBUTTONINFOW:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x20 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_MOUSELEAVE:
		{
			if ( _SendMessageW( g_hWnd_toolbar_tooltip, TTM_GETCURRENTTOOL, 0, NULL ) != 0 )
			{
				g_tb_is_tracking = FALSE;

				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;
				//tti.lpszText = NULL;
				_SendMessageW( g_hWnd_toolbar_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
			}

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			state &= ~0x04;		// Unset the hovered state.

			state &= ~0x08;		// Unset the down state.

			state |= 0x10;		// Index changed.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x20 ) );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			int hot_index = ( int )_SendMessageW( hWnd, TB_GETHOTITEM, 0, 0 );
			if ( hot_index != -1 )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				state |= 0x08;	// Set the down state.

				state &= 0x00FFFF;
				state |= ( ( unsigned char )hot_index << 16 );	// Set the current selected index.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x20 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_LBUTTONUP:
		{
			int hot_index = ( int )_SendMessageW( hWnd, TB_GETHOTITEM, 0, 0 );
			if ( hot_index != -1 )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( state & 0x08 )
				{
					state &= ~0x08;	// Unset the down state.

					state |= 0xFF0000;	// Unset the current selected index.

					_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x20 ) );

					_InvalidateRect( hWnd, NULL, TRUE );
				}
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );
			DWORD old_state = state;

			int hot_index = ( int )_SendMessageW( hWnd, TB_GETHOTITEM, 0, 0 );

			int last_index = ( int )( char )( ( state & 0xFF00 ) >> 8 );
			if ( last_index != hot_index )
			{
				state |= 0x10;	// Index changed.

				if ( hot_index != -1 )
				{
					int tracking_x = GET_X_LPARAM( lParam );
					int tracking_y = GET_Y_LPARAM( lParam );

					if ( tracking_x != g_tb_tracking_x || tracking_y != g_tb_tracking_y )
					{
						g_tb_tracking_x = tracking_x;
						g_tb_tracking_y = tracking_y;

						TBBUTTON tbb;
						_SendMessageW( hWnd, TB_GETBUTTON, hot_index, ( LPARAM )&tbb );
						int len = min( lstrlenW( ( wchar_t * )tbb.iString ), 63 );
						_wmemcpy_s( toolbar_tooltip_text, 64, ( wchar_t * )tbb.iString, len );
						toolbar_tooltip_text[ len ] = 0;

						TOOLINFO tti;
						_memzero( &tti, sizeof( TOOLINFO ) );
						tti.cbSize = sizeof( TOOLINFO );
						tti.hwnd = hWnd;
						tti.uId = ( UINT_PTR )hWnd;
						_SendMessageW( g_hWnd_toolbar_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );

						TRACKMOUSEEVENT tmi;
						tmi.cbSize = sizeof( TRACKMOUSEEVENT );
						tmi.dwFlags = TME_HOVER;
						tmi.hwndTrack = hWnd;
						tmi.dwHoverTime = HOVER_DEFAULT;
						g_tb_is_tracking = _TrackMouseEvent( &tmi );
					}
				}
				else// if ( _SendMessageW( g_hWnd_toolbar_tooltip, TTM_GETCURRENTTOOL, 0, NULL ) != 0 )
				{
					g_tb_is_tracking = FALSE;

					TOOLINFO tti;
					_memzero( &tti, sizeof( TOOLINFO ) );
					tti.cbSize = sizeof( TOOLINFO );
					tti.hwnd = hWnd;
					tti.uId = ( UINT_PTR )hWnd;
					//tti.lpszText = NULL;
					_SendMessageW( g_hWnd_toolbar_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
				}
			}

			if ( hot_index != -1 )
			{
				// Is it currently hovered?
				if ( !( state & 0x04 ) )
				{
					state |= 0x04;	// Set the hovered state.
				}
			}
			else
			{
				state &= ~0x04;		// Unset the hovered state.
			}

			if ( state != old_state )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x20 ) );

				_InvalidateRect( hWnd, NULL, TRUE );
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( state == 0 )
			{
				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x20 ) );
			}

			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_NCPAINT:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x20 ) );
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x20 ) )
			{
				state &= ~0x20;

				bool color_state1 = ( state & 0x01 ? true : false );	// hovered
				bool color_state2 = ( state & 0x02 ? true : false );	// down
				bool is_hovered =	( state & 0x04 ? true : false );
				bool is_down =		( state & 0x08 ? true : false );

				bool index_changed = ( state & 0x10 ? true : false );

				state &= ~0x10;		// Unset the index state.

				int hot_index = ( int )_SendMessageW( hWnd, TB_GETHOTITEM, 0, 0 );
				int last_index = ( int )( char )( ( state & 0xFF00 ) >> 8 );
				int selected_index = ( int )( char )( ( state & 0xFF0000 ) >> 16 );

				COLORREF background_color1a = dm_color_button_normal;
				COLORREF background_color2a = dm_color_button_normal;

				COLORREF background_color1b;

				if ( is_down && hot_index != selected_index )
				{
					hot_index = -1;
				}

				if ( index_changed )
				{
					state &= 0xFF00FF;
					state |= ( ( unsigned char )hot_index << 8 );

					background_color1b = dm_color_button_hover;
				}
				else
				{
					background_color1b = dm_color_button_normal;
				}

				if ( is_down )
				{
					if ( hot_index == selected_index )
					{
						if ( !color_state1 )
						{
							//background_color1a = dm_color_button_normal;
							background_color2a = dm_color_button_pressed;
						}
						else
						{
							background_color1a = dm_color_button_hover;
							background_color2a = dm_color_button_pressed;
						}

						state |= 0x01;		// Turn on the hovered color state.
						state |= 0x02;		// Turn on the down color state.
					}
					else
					{
						state &= ~0x01;	// Turn off the hovered color state.
						state &= ~0x02;	// Turn off the down color state.
					}
				}
				else if ( is_hovered )
				{
					if ( color_state2 )
					{
						background_color1a = dm_color_button_pressed;
						background_color2a = dm_color_button_hover;

						state &= ~0x02;		// Turn off the down color state.
					}
					else
					{
						//background_color1a = dm_color_button_normal;
						background_color2a = dm_color_button_hover;

						state |= 0x01;		// Turn on the hovered color state.
					}
				}
				else
				{
					if ( color_state2 )
					{
						background_color1a = dm_color_button_pressed;
						//background_color2a = dm_color_button_normal;

						state &= ~0x01;	// Turn off the hovered color state.
						state &= ~0x02;	// Turn off the down color state.
					}
					else if ( color_state1 )
					{
						background_color1a = dm_color_button_hover;
						//background_color2a = dm_color_button_normal;

						state &= ~0x01;	// Turn off the hovered color state.
					}
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				BP_ANIMATIONPARAMS animParams;
				_memzero( &animParams, sizeof( animParams ) );
				animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
				animParams.style = BPAS_LINEAR;
				animParams.dwDuration = BUTTON_ANIMATION_DURATION;

				HDC hdcFrom, hdcTo;
				HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &animParams, &hdcFrom, &hdcTo );
				if ( hbpAnimation )
				{
					if ( hdcFrom )
					{
						ToolbarPaint( hWnd, hdcFrom, hot_index, background_color1a, last_index, background_color1b );
					}

					if ( hdcTo )
					{
						ToolbarPaint( hWnd, hdcTo, hot_index, background_color2a, -1, dm_color_button_normal );
					}

					_EndBufferedAnimation( hbpAnimation, TRUE );
				}
				else
				{
					ToolbarPaint( hWnd, hDC, hot_index, background_color1a, last_index, background_color1b );
				}
			}

			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMToolbarSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMChildEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_KILLFOCUS:
		case WM_SETFOCUS:
		{
			HWND hWnd_parent = _GetParent( hWnd );

			_SendMessageW( hWnd_parent, WM_PARENTNOTIFY, ( WPARAM )msg, ( LPARAM )wParam );
		}
		break;

		/*case WM_NCCALCSIZE:
		{
			if ( wParam == FALSE )
			{
				RECT *pRect = ( RECT * )lParam;
				pRect->bottom -= 2;
			}
			else// if ( wParam == TRUE )
			{
				NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
				nccsp->rgrc[ 0 ].bottom -= 2;
			}

			return 0;
		}
		break;*/

		case WM_NCDESTROY:
		{
			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMChildEditSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK DMEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	switch ( msg )
	{
		case WM_ENABLE:
		{
			// Only update if it's visible.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

				if ( wParam == FALSE )
				{
					state |= 0x08;	// Was enabled
				}
				else
				{
					state |= 0x10;	// Was disabled.
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );
			}

			// Don't return here.
		}
		break;

		case WM_KILLFOCUS:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			// This condition handles the case where the control is focused and we switch to a different tab.
			// If we switch back, the previously focused control would have animated. We don't want that to happen.
			if ( _IsWindowVisible( hWnd ) != FALSE )
			{
				state |= 0x20;	// Was focused.
			}

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );
		}
		break;

		case WM_MOUSELEAVE:
		{
			_BufferedPaintStopAllAnimations( hWnd );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			state &= ~0x04;		// Unset the hovered state.

			_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );
		}
		break;

		//case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		{
			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			// Is it currently hovered?
			if ( !( state & 0x04 ) )
			{
				state |= 0x04;	// Set the hovered state.

				_SetPropW( hWnd, L"STATE", ( HANDLE )( state | 0x40 ) );
			}
		}
		break;

		case WM_NCCALCSIZE:
		{
			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( style & ( WS_HSCROLL | WS_VSCROLL ) || !( style & WS_BORDER ) )
			{
				return _DefSubclassProc( hWnd, msg, wParam, lParam );
			}
			else
			{
				if ( wParam == FALSE )
				{
					RECT *pRect = ( RECT * )lParam;
					pRect->top += 2;
					pRect->left += 2;
					pRect->bottom -= 2;
					pRect->right -= 2;
				}
				else// if ( wParam == TRUE )
				{
					NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
					nccsp->rgrc[ 0 ].top += 2;
					nccsp->rgrc[ 0 ].left += 2;
					nccsp->rgrc[ 0 ].bottom -= 2;
					nccsp->rgrc[ 0 ].right -= 2;
				}
			}
			return 0;
		}
		break;

		//case WM_CHILDACTIVATE:
		case WM_PAINT:
		case WM_ERASEBKGND:
		case WM_SETFOCUS:
		case WM_WINDOWPOSCHANGING:
		{
			_SetPropW( hWnd, L"STATE", ( HANDLE )( ( DWORD )_GetPropW( hWnd, L"STATE" ) | 0x40 ) );
		}
		break;

		case WM_NCPAINT:
		{
			LRESULT ret;

			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( style & ( WS_HSCROLL | WS_VSCROLL ) )
			{
				ret  = _DefSubclassProc( hWnd, msg, wParam, lParam );
			}
			else
			{
				ret = 0;
			}

			HDC hDC = _GetWindowDC( hWnd );

			DWORD state = ( DWORD )_GetPropW( hWnd, L"STATE" );

			if ( !_BufferedPaintRenderAnimation( hWnd, hDC ) && ( state & 0x40 ) )
			{
				state &= ~0x40;

				bool color_state1 =	( state & 0x01 ? true : false );	// hovered
				bool color_state2 =	( state & 0x02 ? true : false );	// focused
				bool is_hovered =	( state & 0x04 ? true : false );
				bool was_enabled =	( state & 0x08 ? true : false );
				bool was_disabled =	( state & 0x10 ? true : false );
				bool was_focused =	( state & 0x20 ? true : false );
				bool is_focused =	( hWnd == _GetFocus() ? true : false );

				state &= ~0x08;
				state &= ~0x10;
				state &= ~0x20;

				COLORREF border_color1;
				COLORREF border_color2;

				if ( _IsWindowEnabled( hWnd ) != FALSE )
				{
					if ( is_hovered )
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;
							border_color2 = dm_color_edit_border_hover;

							state |= 0x01;		// Turn on the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else if ( is_focused )
						{
							if ( !color_state2 )
							{
								if ( color_state1 )
								{
									border_color1 = dm_color_edit_border_hover;
								}
								else
								{
									border_color1 = dm_color_edit_border_enabled;
								}

								state |= 0x02;	// Turn on focused color state.
							}
							else
							{
								border_color1 = dm_color_focus_rectangle;
							}

							border_color2 = dm_color_focus_rectangle;
						}
						else if ( !color_state1 )
						{
							border_color1 = dm_color_edit_border_enabled;
							border_color2 = dm_color_edit_border_hover;

							state |= 0x01;		// Turn on the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else
						{
							border_color1 = dm_color_edit_border_hover;
							border_color2 = border_color1;

							state &= ~0x02;		// Turn off the focused color state.
						}
					}
					else
					{
						if ( was_focused )
						{
							border_color1 = dm_color_focus_rectangle;
							border_color2 = dm_color_edit_border_enabled;

							state &= ~0x01;		// Turn off the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else if ( is_focused )
						{
							if ( !color_state2 )
							{
								border_color1 = dm_color_edit_border_enabled;
							}
							else
							{
								border_color1 = dm_color_focus_rectangle;
							}

							border_color2 = dm_color_focus_rectangle;

							state &= ~0x01;		// Turn off the hovered color state.
							state |= 0x02;		// Turn on focused color state.
						}
						else if ( color_state1 )
						{
							border_color1 = dm_color_edit_border_hover;
							border_color2 = dm_color_edit_border_enabled;

							state &= ~0x01;		// Turn off the hovered color state.
							state &= ~0x02;		// Turn off the focused color state.
						}
						else
						{
							border_color1 = dm_color_edit_border_enabled;
							border_color2 = border_color1;

							state &= ~0x02;		// Turn off the focused color state.
						}
					}
				}
				else
				{
					state &= ~0x01;		// Turn off the hovered color state.
					state &= ~0x02;		// Turn off the focused color state.

					border_color1 = dm_color_edit_border_disabled;
					border_color2 = dm_color_edit_border_disabled;
				}

				if ( was_enabled )
				{
					border_color1 = dm_color_edit_border_enabled;
				}
				else if ( was_disabled )
				{
					border_color1 = dm_color_edit_border_disabled;
				}

				_SetPropW( hWnd, L"STATE", ( HANDLE )state );

				//if ( was_enabled || was_disabled || border_color1 != border_color2 )
				{
					BP_ANIMATIONPARAMS animParams;
					_memzero( &animParams, sizeof( animParams ) );
					animParams.cbSize = sizeof( BP_ANIMATIONPARAMS );
					animParams.style = BPAS_LINEAR;

					if ( border_color1 != border_color2 )
					{
						animParams.dwDuration = BUTTON_ANIMATION_DURATION / 2;
					}

					BP_PAINTPARAMS paintParams;
					_memzero( &paintParams, sizeof( BP_PAINTPARAMS ) );
					paintParams.cbSize = sizeof( BP_PAINTPARAMS );
					paintParams.dwFlags = BPPF_ERASE | BPPF_NONCLIENT;

					RECT window_rc;
					_GetWindowRect( hWnd, &window_rc );
					window_rc.right -= window_rc.left;
					window_rc.bottom -= window_rc.top;
					window_rc.top = 0;
					window_rc.left = 0;

					RECT rc_exclude;
					rc_exclude.top = 2;
					rc_exclude.left = 2;
					rc_exclude.right = window_rc.right - 2;
					rc_exclude.bottom = window_rc.bottom - 2;

					paintParams.prcExclude = &rc_exclude;

					HDC hdcFrom, hdcTo;
					HANIMATIONBUFFER hbpAnimation = _BeginBufferedAnimation( hWnd, hDC, &window_rc, BPBF_COMPATIBLEBITMAP, &paintParams, &animParams, &hdcFrom, &hdcTo );
					if ( hbpAnimation )
					{
						if ( hdcFrom )
						{
							EditPaint( hWnd, hdcFrom, border_color1 );
						}

						if ( hdcTo )
						{
							EditPaint( hWnd, hdcTo, border_color2 );
						}

						_EndBufferedAnimation( hbpAnimation, TRUE );
					}
					else
					{
						EditPaint( hWnd, hDC, border_color1 );
					}
				}
				/*else
				{
					EditPaint( hWnd, hDC, border_color1 );
				}*/
			}

			_ReleaseDC( hWnd, hDC );

			return ret;
		}
		break;

		case WM_NCDESTROY:
		{
			_RemovePropW( hWnd, L"STATE" );

			if ( dwRefData != NULL && --( ( SUBCLASS_INFO * )dwRefData )->ref == 0 )
			{
				_RemoveWindowSubclass( hWnd, &DMEditSubProc, uIdSubclass );
			}
		}
		break;
	}

	return _DefSubclassProc( hWnd, msg, wParam, lParam );
}

void SetSubclass( HWND hWnd, SUBCLASSPROC scp, SUBCLASS_INFO *sci )
{
	if ( hWnd != NULL && scp != NULL && sci != NULL )
	{
		if ( sci->id == NULL )
		{
			sci->id = ( UINT_PTR )sci;
		}

		if ( _SetWindowSubclass( hWnd, scp, sci->id, ( DWORD_PTR )sci ) == TRUE )
		{
			++sci->ref;
		}
	}
}

BOOL CALLBACK EnumChildProc( HWND hWnd, LPARAM /*lParam*/ )
{
	wchar_t buf[ 64 ];
	int ret = _GetClassNameW( hWnd, buf, 64 );
	if ( ret == 6 )
	{
		if ( _StrCmpNW( buf, L"Button", 6 ) == 0 )
		{
			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );

			if ( ( style & 0x0000000F ) == BS_AUTOCHECKBOX ||
				 ( style & 0x0000000F ) == BS_AUTORADIOBUTTON )
			{
				// Bits look like this (right to left):
				// Bit 1 0x01: color state 1: hovered
				// Bit 2 0x02: color state 2: down
				// Bit 3 0x04: is hovered
				// Bit 4 0x08: is down
				// Bit 5 0x10: was enabled
				// Bit 6 0x20: was disabled
				// Bit 7 0x40: was checked
				// Bit 8 0x80: in bounds
				// Bit 9 0x100: only paint when we invalidate the region.
				_SetPropW( hWnd, L"STATE", ( HANDLE )0x100 );

				HWND hWnd_parent = _GetParent( hWnd );
				HIMAGELIST hIL = ( HIMAGELIST )_GetPropW( hWnd_parent, L"GLYPHS" );
				if ( hIL == NULL )
				{
					LONG height_width = 0;
					hIL = UpdateButtonGlyphs( hWnd, &height_width );
					if ( hIL != NULL )
					{
						_SetPropW( hWnd_parent, L"GLYPH_HW", ( HANDLE )height_width );

						_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hIL );

						_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )0 );	// Image List
					}
					else
					{
						HTHEME hTheme = _OpenThemeData( hWnd, L"Button" );

						_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hTheme );

						_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )1 );	// Theme
					}
				}

				UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
				_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

				_SetWindowTheme( hWnd, L"", L"" );	// Don't want animation effects.

				SetSubclass( hWnd, &DMCBRBSubProc, &g_sci_cbrb );

				return TRUE;
			}
			else if ( ( style & 0x0000000F ) == BS_GROUPBOX )
			{
				_SetWindowTheme( hWnd, L"", L"" );	// Don't want animation effects.

				SetSubclass( hWnd, &DMGBSubProc, &g_sci_group_box );

				return TRUE;
			}
		}
		else if ( _StrCmpNW( buf, L"Static", 6 ) == 0 )
		{
			LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );
			if ( ( style & 0x000000FF ) == SS_ETCHEDHORZ )
			{
				_SetWindowLongPtrW( hWnd, GWL_EXSTYLE, 0 );	// Remove any extended styles that would create a non-client border.

				RECT rc;
				_GetClientRect( hWnd, &rc );
				_SetWindowPos( hWnd, HWND_TOP, 0, 0, rc.right - rc.left, 1, SWP_FRAMECHANGED | SWP_NOMOVE /*| SWP_NOSIZE*/ | SWP_NOZORDER | SWP_NOOWNERZORDER );

				SetSubclass( hWnd, &DMLineSubProc, &g_sci_line );
			}
			else if ( ( style & 0x000000FF ) != SS_OWNERDRAW )
			{
				SetSubclass( hWnd, &DMStaticSubProc, &g_sci_static );
			}
		}
	}
	else if ( ret == 4 && _StrCmpNW( buf, L"Edit", 4 ) == 0 )
	{
		bool is_child = false;

		// Don't process edit controls that are children of comboboxes.
		HWND hWnd_parent = _GetParent( hWnd );
		if ( hWnd_parent != NULL )
		{
			ret = _GetClassNameW( hWnd_parent, buf, 64 );
			if ( ret == 8 && _StrCmpNW( buf, L"ComboBox", 8 ) == 0 ||
				 ret == 14 && _StrCmpNW( buf, L"SysIPAddress32", 14 ) == 0 ||
				 ret == 12 && _StrCmpNW( buf, L"TreeListView", 12 ) == 0 ||
				 ret == 13 && _StrCmpNW( buf, L"SysTreeView32", 13 ) == 0 )
			{
				SetSubclass( hWnd, &DMChildEditSubProc, &g_sci_edit_child );

				is_child = true;
			}
		}

		if ( !is_child )
		{
			// Bits look like this (right to left):
			// Bit 1 0x01: hovered color state
			// Bit 2 0x02: focused color state
			// Bit 3 0x04: is hovered
			// Bit 4 0x08: was enabled
			// Bit 5 0x10: was disabled
			// Bit 6 0x20: was focused
			// Bit 7 0x40: only paint when we invalidate the region.
			_SetPropW( hWnd, L"STATE", ( HANDLE )0x40 );

			_SetWindowLongPtrW( hWnd, GWL_EXSTYLE, _GetWindowLongPtrW( hWnd, GWL_EXSTYLE ) & ~WS_EX_CLIENTEDGE );
			_SetWindowLongPtrW( hWnd, GWL_STYLE, _GetWindowLongPtrW( hWnd, GWL_STYLE ) | WS_BORDER );

			_AllowDarkModeForWindow( hWnd, true );

			SetSubclass( hWnd, &DMEditSubProc, &g_sci_edit );

			// SWP_FRAMECHANGED forces WM_NCCALCSIZE to get sent.
			_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
		}
	}
	else if ( ret == 7 && _StrCmpNW( buf, L"ListBox", 4 ) == 0 )
	{
		_SetWindowLongPtrW( hWnd, GWL_EXSTYLE, _GetWindowLongPtrW( hWnd, GWL_EXSTYLE ) & ~WS_EX_CLIENTEDGE );
		_SetWindowLongPtrW( hWnd, GWL_STYLE, _GetWindowLongPtrW( hWnd, GWL_STYLE ) | WS_BORDER );
		_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );

		HWND hWnd_parent = _GetParent( hWnd );
		UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
		_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

		SetSubclass( hWnd, &DMListBoxSubProc, &g_sci_list_box );
	}
	else if ( ret == 15 && _StrCmpNW( buf, L"msctls_updown32", 15 ) == 0 )
	{
		// Bits look like this (right to left):
		// Button 1:
		// Bit 1  0x01:  color state 1: hovered
		// Bit 2  0x02:  color state 2: down
		// Bit 3  0x04:  is hovered
		// Bit 4  0x08:  is down
		// Button 2:
		// Bit 5  0x10:  color state 1: hovered
		// Bit 6  0x20:  color state 2: down
		// Bit 7  0x40:  is hovered
		// Bit 8  0x80:  is down
		//
		// Bit 9  0x100: in bounds
		// Bit 10 0x200: was in bounds
		// Bit 11 0x400: was enabled
		// Bit 12 0x800: was disabled
		// Bit 13 0x1000: only paint when we invalidate the region.
		_SetPropW( hWnd, L"STATE", ( HANDLE )0x1000 );

		_SetWindowTheme( hWnd, L"", L"" );	// Don't want animation effects.

		SetSubclass( hWnd, &DMUDSubProc, &g_sci_up_down );

		return TRUE;
	}
	else if ( ret == 14 && _StrCmpNW( buf, L"SysIPAddress32", 14 ) == 0 )
	{
		// Bits look like this (right to left):
		// Bit 1 0x01: hovered color state
		// Bit 2 0x02: focused color state
		// Bit 3 0x04: is hovered
		// Bit 4 0x08: was enabled
		// Bit 5 0x10: was disabled
		// Bit 6 0x20: was focused
		// Bit 7 0x40: only paint when we invalidate the region.
		_SetPropW( hWnd, L"STATE", ( HANDLE )0x40 );

		_SetWindowLongPtrW( hWnd, GWL_EXSTYLE, _GetWindowLongPtrW( hWnd, GWL_EXSTYLE ) & ~WS_EX_CLIENTEDGE );
		_SetWindowLongPtrW( hWnd, GWL_STYLE, _GetWindowLongPtrW( hWnd, GWL_STYLE ) | WS_BORDER );

		SetSubclass( hWnd, &DMIPSubProc, &g_sci_ip );

		// SWP_FRAMECHANGED forces WM_NCCALCSIZE to get sent.
		_SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER );
	}
	else if ( ret == 8 && _StrCmpNW( buf, L"ComboBox", 8 ) == 0 )
	{
		// Bits look like this (right to left):
		// Bit 1 0x01: color state 1: hovered
		// Bit 2 0x02: color state 2: down
		// Bit 3 0x04: is hovered
		// Bit 4 0x08: is down
		// Bit 5 0x10: was enabled
		// Bit 6 0x20: was disabled
		// Bit 7 0x40: was focused
		// Bit 8 0x80: was menu opened
		// Bit 9 0x100: only paint when we invalidate the region.
		_SetPropW( hWnd, L"STATE", ( HANDLE )0x100 );

		HWND hWnd_parent = _GetParent( hWnd );
		UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );

		COMBOBOXINFO cbi;
		cbi.cbSize = sizeof( COMBOBOXINFO );
		_SendMessageW( hWnd, CB_GETCOMBOBOXINFO, 0, ( LPARAM )&cbi );
		if ( cbi.hwndList != NULL )
		{
			_SetPropW( cbi.hwndList, L"DPI", ( HANDLE )current_dpi_dark_mode );

			SetSubclass( cbi.hwndList, &DMListBoxSubProc, &g_sci_list_box );

			_SetWindowTheme( cbi.hwndList, L"", L"" );	// Don't want animation effects.
		}

		_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

		SetSubclass( hWnd, &DMComboBoxSubProc, &g_sci_combo_box );

		// The CBS_OWNERDRAWFIXED flag adds a pixel to the height. Remove it.
		// Normally we'd use WM_MEASUREITEM to correct it, but we don't receive it in the parent's subclass.
		int height = ( int )_SendMessageW( hWnd, CB_GETITEMHEIGHT, ( WPARAM )-1, 0 );
		if ( height != CB_ERR )
		{
			_SendMessageW( hWnd, CB_SETITEMHEIGHT, ( WPARAM )-1, _SCALE_DM_( height ) - 1 );	// This isn't documented? Adjusts the box's height.
		}

		height = ( int )_SendMessageW( hWnd, CB_GETITEMHEIGHT, 0, 0 );
		if ( height != CB_ERR )
		{
			_SendMessageW( hWnd, CB_SETITEMHEIGHT, 0, _SCALE_DM_( height ) );				// Adjust the list item's height.
		}
	}
	else if ( ret == 13 && _StrCmpNW( buf, L"SysTreeView32", 13 ) == 0 )
	{
		_SendMessageW( hWnd, TVM_SETBKCOLOR, 0, ( LPARAM )dm_color_edit_background );
		_SendMessageW( hWnd, TVM_SETTEXTCOLOR, 0, ( LPARAM )dm_color_window_text );

		DWORD style = ( DWORD )_GetWindowLongPtrW( hWnd, GWL_STYLE );
		if ( style & TVS_CHECKBOXES )
		{
			HIMAGELIST hIL = UpdateButtonGlyphs( hWnd, NULL );
			if ( hIL != NULL )
			{
				// The first image is empty, the next two are unchecked and checked.
				// The treeview's unchecked/checked state value (bits 12 through 15) switches between 0x1000 and 0x2000.
				_ImageList_SetImageCount( hIL, 3 );

				_SendMessageW( hWnd, TVM_SETIMAGELIST, TVSIL_STATE, ( LPARAM )hIL );
			}

			HWND hWnd_parent = _GetParent( hWnd );
			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
			_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

			SetSubclass( hWnd, &DMTreeViewSubProc, &g_sci_tree_view );
		}
	}
	else if ( ret == 11 && _StrCmpNW( buf, L"SysHeader32", 11 ) == 0 )
	{
		_AllowDarkModeForWindow( hWnd, true );
		_SetWindowTheme( hWnd, L"ItemsView", NULL );

		return TRUE;
	}
	else if ( ret == 13 && _StrCmpNW( buf, L"SysListView32", 13 ) == 0 )
	{
		_SendMessageW( hWnd, LVM_SETBKCOLOR, 0, ( LPARAM )dm_color_edit_background );
		_SendMessageW( hWnd, LVM_SETTEXTCOLOR, 0, ( LPARAM )dm_color_window_text );

		// We'll draw our own gridlines.
		// All of our listviews are using gridlines so we don't need to keep track of which ones are/aren't.
		DWORD ext_style = ( DWORD )_SendMessageW( hWnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
		ext_style &= ~LVS_EX_GRIDLINES;
		_SendMessageW( hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, ext_style );

		SetSubclass( hWnd, &DMListViewCustomDrawSubProc, &g_sci_list_view );
	}
	else if ( ret == 18 && _StrCmpNW( buf, L"msctls_statusbar32", 18 ) == 0 )
	{
		// The stupid control doesn't darken the tooltip so we have to make our own.
		g_hWnd_status_bar_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, NULL, NULL );
		status_bar_tooltip_text[ 0 ] = 0;

		TOOLINFO tti;
		_memzero( &tti, sizeof( TOOLINFO ) );
		tti.cbSize = sizeof( TOOLINFO );
		tti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
		tti.hwnd = hWnd;
		tti.uId = ( UINT_PTR )hWnd;

		_SendMessageW( g_hWnd_status_bar_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
		_SendMessageW( g_hWnd_status_bar_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );
		//_SendMessageW( g_hWnd_status_bar_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );

		SetSubclass( hWnd, &DMStatusBarSubProc, &g_sci_status_bar );
	}
	else if ( ret == 15 && _StrCmpNW( buf, L"ToolbarWindow32", 15 ) == 0 )
	{
		// Bits look like this (right to left):
		// Bit 1 0x01: color state 1: hovered
		// Bit 2 0x02: color state 2: down
		// Bit 3 0x04: is hovered
		// Bit 4 0x08: is down
		// Bit 5 0x10: index changed
		// Bit 6 0x20: only paint when we invalidate the region.
		// .....
		// Bits 9-16  0xFF00:   hover index
		// Bits 17-24 0xFF0000: selected index
		_SetPropW( hWnd, L"STATE", ( HANDLE )0xFFFF20 );

		_SetWindowTheme( hWnd, L"", L"" );	// Don't want animation effects.

		g_hWnd_toolbar_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, NULL, NULL );
		toolbar_tooltip_text[ 0 ] = 0;

		TOOLINFO tti;
		_memzero( &tti, sizeof( TOOLINFO ) );
		tti.cbSize = sizeof( TOOLINFO );
		tti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
		tti.hwnd = hWnd;
		tti.uId = ( UINT_PTR )hWnd;

		_SendMessageW( g_hWnd_toolbar_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
		_SendMessageW( g_hWnd_toolbar_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );

		SetSubclass( hWnd, &DMToolbarSubProc, &g_sci_toolbar );

		return TRUE;
	}
	else if ( ret == 15 && _StrCmpNW( buf, L"SysTabControl32", 15 ) == 0 )
	{
		SetSubclass( hWnd, &DMTabControlSubProc, &g_sci_tab );
	}
	else if ( ( ret == 17 && _StrCmpNW( buf, L"class_general_tab", 17 ) == 0 ) ||
			  ( ret == 20 && _StrCmpNW( buf, L"class_appearance_tab", 20 ) == 0 ) ||
			  ( ret == 20 && _StrCmpNW( buf, L"class_connection_tab", 20 ) == 0 ) ||
			  ( ret == 20 && _StrCmpNW( buf, L"class_web_server_tab", 20 ) == 0 ) ||
			  ( ret == 13 && _StrCmpNW( buf, L"class_ftp_tab", 13 ) == 0 ) ||
			  ( ret == 14 && _StrCmpNW( buf, L"class_sftp_tab", 14 ) == 0 ) ||
			  ( ret == 18 && _StrCmpNW( buf, L"class_sftp_fps_tab", 18 ) == 0 ) ||
			  ( ret == 19 && _StrCmpNW( buf, L"class_sftp_keys_tab", 19 ) == 0 ) ||
			  ( ret == 15 && _StrCmpNW( buf, L"class_proxy_tab", 15 ) == 0 ) ||
			  ( ret == 18 && _StrCmpNW( buf, L"class_advanced_tab", 18 ) == 0 ) )
	{
		SetSubclass( hWnd, &DMControlColorSubProc, &g_sci_control_color );
	}
	/*else if ( ret == 16 && _StrCmpNW( buf, L"tooltips_class32", 16 ) == 0 )
	{
	}*/

	_SetWindowTheme( hWnd, L"DarkMode_Explorer", NULL );

	return TRUE;
}

BOOL CALLBACK EnumTLWProc( HWND hWnd, LPARAM /*lParam*/ )
{
	wchar_t buf[ 64 ];
	int ret = _GetClassNameW( hWnd, buf, 64 );
	if ( ret == 16 && _StrCmpNW( buf, L"tooltips_class32", 16 ) == 0 )
	{
		_SetWindowTheme( hWnd, L"DarkMode_Explorer", NULL );
	}
	else if ( ( ret == 21 && _StrCmpNW( buf, L"class_http_downloader", 21 ) == 0 ) ||
			  ( ret == 14 && _StrCmpNW( buf, L"class_add_urls", 14 ) == 0 ) ||
			  ( ret == 13 && _StrCmpNW( buf, L"class_options", 13 ) == 0 ) ||
			  ( ret == 21 && _StrCmpNW( buf, L"class_update_download", 21 ) == 0 ) ||
			  ( ret == 12 && _StrCmpNW( buf, L"class_search", 12 ) == 0 ) ||
			  ( ret == 26 && _StrCmpNW( buf, L"class_download_speed_limit", 26 ) == 0 ) ||
			  ( ret == 18 && _StrCmpNW( buf, L"class_site_manager", 18 ) == 0 ) ||
			  ( ret == 23 && _StrCmpNW( buf, L"class_check_for_updates", 23 ) == 0 ) ||
			  ( ret == 18 && _StrCmpNW( buf, L"class_site_manager", 18 ) == 0 ) ||
			  ( ret == 24 && _StrCmpNW( buf, L"class_fingerprint_prompt", 24 ) == 0 ) ||
			  ( ret == 18 && _StrCmpNW( buf, L"class_add_category", 18 ) == 0 ) ||
			  ( ret == 13 && _StrCmpNW( buf, L"SysTreeView32", 13 ) == 0 ) ||
			  ( ret == 12 && _StrCmpNW( buf, L"TreeListView", 12 ) == 0 ) )
	{
		BOOL dark = TRUE;
		/*WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof( dark ) };
		_SetWindowCompositionAttribute( hWnd, &data );*/

		_DwmSetWindowAttribute( hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof( dark ) );

		SetSubclass( hWnd, &DMControlColorSubProc, &g_sci_control_color );

		_SetWindowTheme( hWnd, L"DarkMode_Explorer", NULL );
	}
	else if ( ( ret == 17 && _StrCmpNW( buf, L"class_cmessagebox", 17 ) == 0 ) ||
			  ( ret == 19 && _StrCmpNW( buf, L"class_cmessageboxdc", 19 ) == 0 ) )
	{
		BOOL dark = TRUE;
		/*WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof( dark ) };
		_SetWindowCompositionAttribute( hWnd, &data );*/

		_DwmSetWindowAttribute( hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof( dark ) );

		SetSubclass( hWnd, &DMMsgBoxColorSubProc, &g_sci_msg_box );

		_SetWindowTheme( hWnd, L"DarkMode_Explorer", NULL );
	}

	return TRUE;
}

BOOL CALLBACK EnumMsgBoxChildProc( HWND hWnd, LPARAM /*lParam*/ )
{
	wchar_t buf[ 64 ];
	int ret = _GetClassNameW( hWnd, buf, 64 );
	if ( ret == 6 && _StrCmpNW( buf, L"Button", 6 ) == 0 )
	{
		LONG_PTR style = _GetWindowLongPtrW( hWnd, GWL_STYLE );

		if ( ( style & 0x0000000F ) == BS_AUTOCHECKBOX )
		{
			// Bits look like this (right to left):
			// Bit 1 0x01: color state 1: hovered
			// Bit 2 0x02: color state 2: down
			// Bit 3 0x04: is hovered
			// Bit 4 0x08: is down
			// Bit 5 0x10: was enabled
			// Bit 6 0x20: was disabled
			// Bit 7 0x40: was checked
			// Bit 8 0x80: in bounds
			// Bit 9 0x100: only paint when we invalidate the region.
			_SetPropW( hWnd, L"STATE", ( HANDLE )0x100 );

			HWND hWnd_parent = _GetParent( hWnd );
			HIMAGELIST hIL = ( HIMAGELIST )_GetPropW( hWnd_parent, L"GLYPHS" );
			if ( hIL == NULL )
			{
				LONG height_width = 0;
				hIL = UpdateButtonGlyphs( hWnd, &height_width );
				if ( hIL != NULL )
				{
					_SetPropW( hWnd_parent, L"GLYPH_HW", ( HANDLE )height_width );

					_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hIL );

					_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )0 );	// Image List
				}
				else
				{
					HTHEME hTheme = _OpenThemeData( hWnd, L"Button" );

					_SetPropW( hWnd_parent, L"GLYPHS", ( HANDLE )hTheme );

					_SetPropW( hWnd_parent, L"GLYPH_TYPE", ( HANDLE )1 );	// Theme
				}
			}

			UINT current_dpi_dark_mode = ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 );
			_SetPropW( hWnd, L"DPI", ( HANDLE )current_dpi_dark_mode );

			SetSubclass( hWnd, &DMCBRBSubProc, &g_sci_cbrb );
		}

		_SetWindowTheme( hWnd, L"DarkMode_Explorer", NULL );
	}

	return TRUE;
}

bool InitDarkMode()
{
	if ( dark_mode_state != DARK_MODE_STATE_SHUTDOWN )
	{
		return true;
	}

	hModule_dm_ntdll = LoadLibraryDEMW( L"ntdll.dll" );

	if ( hModule_dm_ntdll == NULL )
	{
		return false;
	}

	// Undocumented
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_ntdll, ( void ** )&_RtlGetNtVersionNumbers, "RtlGetNtVersionNumbers" ) )

	DWORD major, minor;
	_RtlGetNtVersionNumbers( &major, &minor, &g_dm_buildNumber );
	g_dm_buildNumber &= ~0xF0000000;

	// Ensure the build we're on supports dark mode.
	if ( major == 10 && minor == 0 && g_dm_buildNumber >= WINDOWS_BUILD_1809 )
	{
		hModule_dm_user32 = LoadLibraryDEMW( L"user32.dll" );

		if ( hModule_dm_user32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_GetMenuItemInfoW, "GetMenuItemInfoW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_GetMenuBarInfo, "GetMenuBarInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_EnumChildWindows, "EnumChildWindows" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_EnumThreadWindows, "EnumThreadWindows" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_GetClassNameW, "GetClassNameW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_RedrawWindow, "RedrawWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_SetPropW, "SetPropW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_RemovePropW, "RemovePropW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_GetPropW, "GetPropW" ) )
		// Undocumented
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_user32, ( void ** )&_SetWindowCompositionAttribute, "SetWindowCompositionAttribute" ) )

		//

		hModule_dm_comctl32 = LoadLibraryDEMW( L"comctl32.dll" );

		if ( hModule_dm_comctl32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_ImageList_Draw, "ImageList_Draw" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_ImageList_SetImageCount, "ImageList_SetImageCount" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_ImageList_GetIcon, "ImageList_GetIcon" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_SetWindowSubclass, "SetWindowSubclass" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_DefSubclassProc, "DefSubclassProc" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_comctl32, ( void ** )&_RemoveWindowSubclass, "RemoveWindowSubclass" ) )

		//

		hModule_dm_gdi32 = LoadLibraryDEMW( L"gdi32.dll" );

		if ( hModule_dm_gdi32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_gdi32, ( void ** )&_ExcludeClipRect, "ExcludeClipRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_gdi32, ( void ** )&_SetBkColor, "SetBkColor" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_gdi32, ( void ** )&_Polygon, "Polygon" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_gdi32, ( void ** )&_ExtCreatePen, "ExtCreatePen" ) )

		//

		hModule_dm_uxtheme = LoadLibraryDEMW( L"uxtheme.dll" );

		if ( hModule_dm_uxtheme == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_SetWindowTheme, "SetWindowTheme" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_DrawThemeTextEx, "DrawThemeTextEx" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_GetThemeRect, "GetThemeRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_BufferedPaintInit, "BufferedPaintInit" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_BufferedPaintUnInit, "BufferedPaintUnInit" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_BufferedPaintRenderAnimation, "BufferedPaintRenderAnimation" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_BeginBufferedAnimation, "BeginBufferedAnimation" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_EndBufferedAnimation, "EndBufferedAnimation" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_BufferedPaintStopAllAnimations, "BufferedPaintStopAllAnimations" ) )

		// Undocumented
		// 1809 17763
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_OpenNcThemeData, MAKEINTRESOURCEA( 49 ) ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_RefreshImmersiveColorPolicyState, MAKEINTRESOURCEA( 104 ) ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_GetIsImmersiveColorUsingHighContrast, MAKEINTRESOURCEA( 106 ) ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_ShouldAppsUseDarkMode, MAKEINTRESOURCEA( 132 ) ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_AllowDarkModeForWindow, MAKEINTRESOURCEA( 133 ) ) )
		if ( g_dm_buildNumber < WINDOWS_BUILD_1909 )
		{
			VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_AllowDarkModeForApp, MAKEINTRESOURCEA( 135 ) ) )	// 135 in 1809
		}
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_FlushMenuThemes, MAKEINTRESOURCEA( 136 ) ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_IsDarkModeAllowedForWindow, MAKEINTRESOURCEA( 137 ) ) )
		// 1903 18362
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_ShouldSystemUseDarkMode, MAKEINTRESOURCEA( 138 ) ) )
		if ( g_dm_buildNumber >= WINDOWS_BUILD_1909 )
		{
			VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_SetPreferredAppMode, MAKEINTRESOURCEA( 135 ) ) )	// 135 in 1903
		}
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_uxtheme, ( void ** )&_IsDarkModeAllowedForApp, MAKEINTRESOURCEA( 139 ) ) )

		//

		hModule_dm_dwmapi = LoadLibraryDEMW( L"dwmapi.dll" );

		if ( hModule_dm_dwmapi == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_dm_dwmapi, ( void ** )&_DwmSetWindowAttribute, "DwmSetWindowAttribute" ) )

		if ( g_dm_buildNumber < WINDOWS_BUILD_20H1 )
		{
			DWMWA_USE_IMMERSIVE_DARK_MODE = 19;
		}
		else
		{
			DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
		}

		//

		g_hBrush_window_background = _CreateSolidBrush( dm_color_window_background );
		g_hBrush_edit_background = _CreateSolidBrush( dm_color_edit_background );

		CBS_DARK_MODE = CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;
		LBS_DARK_MODE = LBS_OWNERDRAWFIXED | LBS_HASSTRINGS;

		_BufferedPaintInit();

		dark_mode_state = DARK_MODE_STATE_RUNNING;

		return true;
	}
	else
	{
		return false;
	}
}

bool UninitDarkMode()
{
	// Clean up objects.

	if ( g_hTheme_menu != NULL )
	{
		_CloseThemeData( g_hTheme_menu );
	}

	if ( hTheme_status != NULL )
	{
		_CloseThemeData( hTheme_status );
	}

	if ( g_hBrush_window_background != NULL )
	{
		_DeleteObject( g_hBrush_window_background );
	}

	if ( g_hBrush_edit_background != NULL )
	{
		_DeleteObject( g_hBrush_edit_background );
	}

	// Unload libraries.

	if ( hModule_dm_user32 != NULL )
	{
		FreeLibrary( hModule_dm_user32 );
	}

	if ( hModule_dm_comctl32 != NULL )
	{
		FreeLibrary( hModule_dm_comctl32 );
	}

	if ( hModule_dm_gdi32 != NULL )
	{
		FreeLibrary( hModule_dm_gdi32 );
	}

	if ( hModule_dm_ntdll != NULL )
	{
		FreeLibrary( hModule_dm_ntdll );
	}

	if ( hModule_dm_uxtheme != NULL )
	{
		_BufferedPaintUnInit();

		FreeLibrary( hModule_dm_uxtheme );
	}

	if ( hModule_dm_dwmapi != NULL )
	{
		FreeLibrary( hModule_dm_dwmapi );
	}

	//

	dark_mode_state = DARK_MODE_STATE_SHUTDOWN;

	return true;
}

#endif
