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
#include "lite_winmm.h"
#include "lite_uxtheme.h"

#include "menus.h"
#include "string_tables.h"
#include "cmessagebox.h"
#include "wnd_proc.h"

#include "dark_mode.h"

#define BTN_YES			1001
#define BTN_NO			1002
#define BTN_OK			1003
#define BTN_RENAME		1004
#define BTN_OVERWRITE	1005
#define BTN_CONTINUE	1006
#define BTN_RESTART		1007
#define BTN_SKIP		1008

CRITICAL_SECTION cmessagebox_prompt_cs;	// Guard access to the tray menu when creating a cmessagebox.
unsigned int cmessagebox_prompt_count = 0;

#define BTN_AREA_HEIGHT			42

#define ICON_X_POS				21
#define ICON_Y_POS				23

//#define ICON_WIDTH				32
//#define ICON_HEIGHT				32

#define ICON_PADDING			10
#define TEXT_PADDING			10

bool g_glyph_size_set = false;
SIZE g_cb_glyph_size;

#define _SCALE_MB_( x )						MulDiv( ( x ), cmb_info->current_dpi_message_box, USER_DEFAULT_SCREEN_DPI )

bool InitializeCMessageBox( HINSTANCE hInstance )
{
	// Set/Get won't work after a window has been created. Have to register separate classes.
	//_SetClassLongPtrW( hWnd, GCL_STYLE, _GetClassLongPtrW( hWnd, GCL_STYLE ) | CS_NOCLOSE );
	//_SetClassLongPtrW( hWnd, GCL_STYLE, _GetClassLongPtrW( hWnd, GCL_STYLE ) & ~CS_NOCLOSE );

	// Initialize our window class.
	WNDCLASSEX wcex;
	_memzero( &wcex, sizeof( WNDCLASSEX ) );
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style			= 0;//CS_VREDRAW | CS_HREDRAW;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hCursor		= _LoadCursorW( NULL, IDC_ARROW );
	wcex.hbrBackground	= ( HBRUSH )( COLOR_3DFACE + 1 );
	wcex.cbWndExtra		= sizeof( LONG_PTR );		// We're going to pass each message window an info struct pointer.
	wcex.lpfnWndProc	= CustomMessageBoxWndProc;

#ifdef ENABLE_DARK_MODE
	if ( g_use_dark_mode )
	{
		wcex.hbrBackground = g_hBrush_window_background;
	}
#endif

	wcex.lpszClassName	= L"class_cmessagebox";
	if ( !_RegisterClassExW( &wcex ) )
	{
		return false;
	}

	// Disable the close button.
	//wcex.style		   |= CS_NOCLOSE;
	wcex.lpszClassName	= L"class_cmessageboxdc";
	if ( !_RegisterClassExW( &wcex ) )
	{
		return false;
	}

	//wcex.style		   &= ~CS_NOCLOSE;
	wcex.lpfnWndProc	= FingerprintPromptWndProc;
	wcex.lpszClassName	= L"class_fingerprint_prompt";
	if ( !_RegisterClassExW( &wcex ) )
	{
		return false;
	}

	return true;
}

int CMessageBoxW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType )
{
	int ret = 0;

	// This struct is created for each instance of a messagebox window.
	CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )GlobalAlloc( GPTR, sizeof( CMSGBOX_INFO ) );
	if ( cmb_info != NULL )
	{
		cmb_info->cmsgbox_message = GlobalStrDupW( lpText );
		cmb_info->type = uType;

		bool use_theme = true;

		#ifndef UXTHEME_USE_STATIC_LIB
		if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
		{
			use_theme = InitializeUXTheme();
		}
		#endif

		if ( use_theme )
		{
			if ( !g_glyph_size_set )
			{
				g_glyph_size_set = true;

				HTHEME hTheme = _OpenThemeData( hWnd, L"Button" );
				_GetThemePartSize( hTheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &g_cb_glyph_size );
				_CloseThemeData( hTheme );
			}
		}
		else
		{
			g_cb_glyph_size.cx = g_cb_glyph_size.cy = 0;
		}

		cmb_info->use_theme = use_theme && ( _IsThemeActive() == TRUE ? true : false );

		HWND hWnd_cmsgbox = _CreateWindowExW( WS_EX_DLGMODALFRAME, ( ( ( uType & 0x0F ) == CMB_YESNO ||
																	   ( uType & 0x0F ) == CMB_YESNOALL ||
																	   ( uType & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
																	   ( uType & 0x0F ) == CMB_CONTINUERESTARTSKIPALL ||
																	   ( uType & 0x0F ) == CMB_OKALL ) ? L"class_cmessageboxdc" : L"class_cmessagebox" ), lpCaption, WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 100, 100, hWnd, NULL, NULL, ( LPVOID )cmb_info );
		if ( hWnd_cmsgbox != NULL )
		{
			EnterCriticalSection( &cmessagebox_prompt_cs );

			if ( cmessagebox_prompt_count == 0 )
			{
				//_EnableMenuItem( g_hMenuSub_tray, MENU_RESTORE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_OPTIONS, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_ADD_URLS, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_EXIT, MF_GRAYED );
			}

			++cmessagebox_prompt_count;

			LeaveCriticalSection( &cmessagebox_prompt_cs );

			// Force the window to be painted.
			_ShowWindow( hWnd_cmsgbox, SW_SHOW );

			if ( _IsIconic( hWnd ) )
			{
				_FlashWindow( hWnd, TRUE );
			}

			// CMessageBox message loop:
			MSG msg;
			while ( _GetMessageW( &msg, NULL, 0, 0 ) > 0 )
			{
				// Destroy the window and exit the loop.
				if ( msg.message == WM_DESTROY_CMSGBOX )
				{
					ret = ( int )msg.wParam;	// The messagebox's return value before being destroyed.

					_DestroyWindow( msg.hwnd );

					break;
				}

				if ( g_hWnd_active == NULL || !_IsDialogMessageW( g_hWnd_active, &msg ) )	// Checks tab stops.
				{
					_TranslateMessage( &msg );
					_DispatchMessageW( &msg );
				}
			}

			EnterCriticalSection( &cmessagebox_prompt_cs );

			--cmessagebox_prompt_count;

			if ( cmessagebox_prompt_count == 0 )
			{
				//_EnableMenuItem( g_hMenuSub_tray, MENU_RESTORE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_OPTIONS, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_ADD_URLS, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_EXIT, MF_ENABLED );
			}

			LeaveCriticalSection( &cmessagebox_prompt_cs );
		}
		else
		{
			GlobalFree( cmb_info->cmsgbox_message );
			GlobalFree( cmb_info );
		}
	}

	return ret;
}

int CPromptW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, void *data )
{
	int ret = 0;

	// This struct is created for each instance of a messagebox window.
	CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )GlobalAlloc( GPTR, sizeof( CPROMPT_INFO ) );
	if ( cp_info != NULL )
	{
		cp_info->cp_message = GlobalStrDupW( lpText );
		cp_info->type = uType;
		cp_info->data = data;

		HWND hWnd_cmsgbox = _CreateWindowExW( WS_EX_DLGMODALFRAME, L"class_fingerprint_prompt", lpCaption, WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 600, 253, hWnd, NULL, NULL, ( LPVOID )cp_info );
		if ( hWnd_cmsgbox != NULL )
		{
			EnterCriticalSection( &cmessagebox_prompt_cs );

			if ( cmessagebox_prompt_count == 0 )
			{
				//_EnableMenuItem( g_hMenuSub_tray, MENU_RESTORE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_OPTIONS, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_ADD_URLS, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_EXIT, MF_GRAYED );
			}

			++cmessagebox_prompt_count;

			LeaveCriticalSection( &cmessagebox_prompt_cs );

			// Force the window to be painted.
			_ShowWindow( hWnd_cmsgbox, SW_SHOW );

			if ( _IsIconic( hWnd ) )
			{
				_FlashWindow( hWnd, TRUE );
			}

			// CMessageBox message loop:
			MSG msg;
			while ( _GetMessageW( &msg, NULL, 0, 0 ) > 0 )
			{
				// Destroy the window and exit the loop.
				if ( msg.message == WM_DESTROY_CMSGBOX )
				{
					ret = ( int )msg.wParam;	// The messagebox's return value before being destroyed.

					_DestroyWindow( msg.hwnd );

					break;
				}

				if ( g_hWnd_active == NULL || !_IsDialogMessageW( g_hWnd_active, &msg ) )	// Checks tab stops.
				{
					_TranslateMessage( &msg );
					_DispatchMessageW( &msg );
				}
			}

			EnterCriticalSection( &cmessagebox_prompt_cs );

			--cmessagebox_prompt_count;

			if ( cmessagebox_prompt_count == 0 )
			{
				//_EnableMenuItem( g_hMenuSub_tray, MENU_RESTORE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_OPTIONS, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_ADD_URLS, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_tray, MENU_EXIT, MF_ENABLED );
			}

			LeaveCriticalSection( &cmessagebox_prompt_cs );
		}
		else
		{
			GlobalFree( cp_info->cp_message );
			GlobalFree( cp_info );
		}
	}

	return ret;
}

void CMBGetClientRect( HWND hWnd, CMSGBOX_INFO *cmb_info, RECT *rc )
{
	// Limit the width of the text area that's being drawn.

	if ( cmb_info == NULL || rc == NULL )
	{
		return;
	}

	rc->left = rc->top = 0;

	HMONITOR hMon = _MonitorFromWindow( _GetParent( hWnd ), MONITOR_DEFAULTTONEAREST );
	MONITORINFO mi;
	mi.cbSize = sizeof( MONITORINFO );
	_GetMonitorInfoW( hMon, &mi );

	TEXTMETRIC tm;

	HDC hDC = _GetDC( hWnd );
	HFONT ohf = ( HFONT )_SelectObject( hDC, cmb_info->hFont_message_box );
	_GetTextMetricsW( hDC, &tm );
	_DeleteObject( ohf );

	// Calculate the height and width of the message.
	if ( cmb_info->cmsgbox_message != NULL )
	{
		cmb_info->msg_rc.right = mi.rcWork.right - mi.rcWork.left;
		cmb_info->msg_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &cmb_info->msg_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
	}

	int cb_width;

	if ( cmb_info->cb_message != NULL )
	{
		cmb_info->cb_rc.right = mi.rcWork.right - mi.rcWork.left;
		cmb_info->cb_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cmb_info->cb_message, -1, &cmb_info->cb_rc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE );

		cb_width = ( cmb_info->cb_rc.right - cmb_info->cb_rc.left ) + _SCALE_MB_( g_cb_glyph_size.cx ) + tm.tmAveCharWidth;

		cmb_info->cb_rc.right += ( _SCALE_MB_( g_cb_glyph_size.cx ) + tm.tmAveCharWidth );
	}
	else
	{
		cb_width = 0;
	}

	int msgbox_width;

	if ( cmb_info->type & 0xF0 )	// Icon.
	{
		msgbox_width = _SCALE_MB_( ICON_X_POS ) + cmb_info->icon_width + _SCALE_MB_( ICON_PADDING ) + ( cmb_info->msg_rc.right - cmb_info->msg_rc.left ) + _SCALE_MB_( ICON_X_POS );
	}
	else
	{
		msgbox_width = _SCALE_MB_( TEXT_PADDING ) + ( cmb_info->msg_rc.right - cmb_info->msg_rc.left ) + _SCALE_MB_( TEXT_PADDING );
	}

	int twidth = MulDiv( _SCALE_MB_( 278 ), max( tm.tmAveCharWidth, _SCALE_MB_( 6 ) ), _SCALE_MB_( 4 ) );	// DLU to px.
	if ( twidth < msgbox_width ) { msgbox_width = twidth; }

	// Minimum widths for the windows.
	if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
		 ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
	{
		if ( msgbox_width < ( _SCALE_MB_( 313 ) + cb_width ) )
		{
			msgbox_width = _SCALE_MB_( 313 ) + cb_width;	// ( 85 + 8 + 85 + 8 + 85 + 10 ) + 32
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
	{
		if ( msgbox_width < ( _SCALE_MB_( 210 ) + cb_width ) )
		{
			msgbox_width = _SCALE_MB_( 210 ) + cb_width;
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_YESNO )
	{
		if ( msgbox_width < _SCALE_MB_( 210 ) )
		{
			msgbox_width = _SCALE_MB_( 210 );	// ( 80 + 8 + 80 + 10 ) + 32
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_OKALL )
	{
		if ( msgbox_width < ( _SCALE_MB_( 122 ) + cb_width ) )
		{
			msgbox_width = _SCALE_MB_( 122 ) + cb_width;
		}
	}
	else	// CMB_OK or anything that's unsupported.
	{
		if ( msgbox_width < _SCALE_MB_( 122 ) )
		{
			msgbox_width = _SCALE_MB_( 122 );	// ( 80 + 10 ) + 32
		}
	}

	rc->right = msgbox_width;

	/*int tmsgbox_width = msgbox_width;

	int twidth = MulDiv( 278, tm.tmAveCharWidth, 4 );	// DLU to px.
	if ( twidth < msgbox_width ) { tmsgbox_width = twidth; }
	twidth = MulDiv( mi.rcWork.right - mi.rcWork.left, 5, 8 );
	if ( twidth < msgbox_width ) { tmsgbox_width = twidth; }
	twidth = MulDiv( mi.rcWork.right - mi.rcWork.left, 3, 4 );
	if ( twidth < msgbox_width ) { tmsgbox_width = twidth; }
	twidth = MulDiv( mi.rcWork.right - mi.rcWork.left, 7, 8 );
	if ( twidth < msgbox_width ) { tmsgbox_width = twidth; }

	msgbox_width = tmsgbox_width;*/

	if ( cmb_info->type & 0xF0 )	// Icon.
	{
		cmb_info->msg_rc.right = msgbox_width - ( _SCALE_MB_( ICON_X_POS ) + cmb_info->icon_width + _SCALE_MB_( ICON_PADDING ) + _SCALE_MB_( ICON_X_POS ) );
	}
	else
	{
		cmb_info->msg_rc.right = msgbox_width - ( _SCALE_MB_( TEXT_PADDING ) + _SCALE_MB_( TEXT_PADDING ) );
	}

	// Recalculate the text message area based on the new message box width.
	if ( cmb_info->cmsgbox_message != NULL )
	{
		cmb_info->msg_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &cmb_info->msg_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
	}
	_ReleaseDC( hWnd, hDC );

	rc->bottom = _SCALE_MB_( ICON_Y_POS ) + max( ( cmb_info->msg_rc.bottom - cmb_info->msg_rc.top ), cmb_info->icon_height ) + _SCALE_MB_( ICON_Y_POS ) + _SCALE_MB_( BTN_AREA_HEIGHT );
}

LRESULT CALLBACK CustomMessageBoxWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )( ( CREATESTRUCT * )lParam )->lpCreateParams;
			if ( cmb_info != NULL )
			{
				// Pass our messagebox info to the window.
				_SetWindowLongPtrW( hWnd, 0, ( LONG_PTR )cmb_info );

				HWND hWnd_parent = _GetParent( hWnd );

				cmb_info->current_dpi_message_box = __GetDpiForWindow( hWnd );
				cmb_info->last_dpi_message_box = ( cmb_info->current_dpi_message_box == ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 ) ? cmb_info->current_dpi_message_box : 0 );
				cmb_info->hFont_message_box = UpdateFont( cmb_info->current_dpi_message_box );

				RECT rc;
				_GetClientRect( hWnd, &rc );

				cmb_info->hWnd_static_icon = _CreateWindowW( WC_STATIC, NULL, SS_ICON | WS_CHILD | ( ( cmb_info->type & 0xF0 ) != 0 ? WS_VISIBLE : 0 ), ICON_X_POS, ICON_Y_POS, 0, 0, hWnd, NULL, NULL, NULL );

				HICON hIcon = NULL;

				if ( cmb_info->type & 0xF0 )
				{
					cmb_info->icon_width = _GetSystemMetrics( SM_CXICON );
					cmb_info->icon_height = _GetSystemMetrics( SM_CYICON );

					bool play = true;
					#ifndef WINMM_USE_STATIC_LIB
						if ( winmm_state == WINMM_STATE_SHUTDOWN )
						{
							play = InitializeWinMM();
						}
					#endif

					LPCWSTR icon_name = NULL;
					LPCWSTR sound_name = NULL;

					switch ( ( cmb_info->type & 0xF0 ) )	// Icon type.
					{
						case CMB_ICONASTERISK:		{ icon_name = MAKEINTRESOURCE( 104 ); sound_name = ( LPCTSTR )SND_ALIAS_SYSTEMASTERISK; } break;	// IDI_ASTERISK
						case CMB_ICONEXCLAMATION:	{ icon_name = MAKEINTRESOURCE( 101 ); sound_name = ( LPCTSTR )SND_ALIAS_SYSTEMEXCLAMATION; } break;	// IDI_EXCLAMATION
						case CMB_ICONQUESTION:		{ icon_name = MAKEINTRESOURCE( 102 ); sound_name = ( LPCTSTR )SND_ALIAS_SYSTEMQUESTION; } break;	// IDI_QUESTION
						case CMB_ICONHAND:			{ icon_name = MAKEINTRESOURCE( 103 ); sound_name = ( LPCTSTR )SND_ALIAS_SYSTEMHAND; } break;	// IDI_HAND
					}

					if ( icon_name != NULL )
					{
						hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), icon_name, IMAGE_ICON, cmb_info->icon_width, cmb_info->icon_height, LR_SHARED );
					}

					if ( play && sound_name != NULL )
					{
						_PlaySoundW( sound_name, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT );
					}
				}

				if ( hIcon != NULL )
				{
					_SendMessageW( cmb_info->hWnd_static_icon, STM_SETICON, ( WPARAM )hIcon, 0 );
				}

				cmb_info->hWnd_static_message = _CreateWindowW( WC_STATIC, cmb_info->cmsgbox_message, SS_EDITCONTROL | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

				if ( ( cmb_info->type & 0x0F ) == CMB_OKALL ||
					 ( cmb_info->type & 0x0F ) == CMB_YESNOALL ||
					 ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
					 ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					cmb_info->cb_message = ( ( cmb_info->type & 0x0F ) == CMB_OKALL ? ST_V_Skip_remaining_messages : ST_V_Remember_choice );

					cmb_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, cmb_info->cb_message, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
				}

				if ( ( cmb_info->type & 0x0F ) == CMB_YESNO || ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
				{
					cmb_info->hWnd_btn_1 = _CreateWindowW( WC_BUTTON, ST_V_Yes, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_YES, NULL, NULL );
					cmb_info->hWnd_btn_2 = _CreateWindowW( WC_BUTTON, ST_V_No, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_NO, NULL, NULL );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
				{
					cmb_info->hWnd_btn_1 = _CreateWindowW( WC_BUTTON, ST_V_Rename, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_RENAME, NULL, NULL );
					cmb_info->hWnd_btn_2 = _CreateWindowW( WC_BUTTON, ST_V_Overwrite, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_OVERWRITE, NULL, NULL );
					cmb_info->hWnd_btn_3 = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					cmb_info->hWnd_btn_1 = _CreateWindowW( WC_BUTTON, ST_V_Continue, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CONTINUE, NULL, NULL );
					cmb_info->hWnd_btn_2 = _CreateWindowW( WC_BUTTON, ST_V_Restart, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_RESTART, NULL, NULL );
					cmb_info->hWnd_btn_3 = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );
				}
				else	// CMB_OK/ALL or anything that's unsupported.
				{
					cmb_info->hWnd_btn_1 = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_OK, NULL, NULL );
				}

				_SendMessageW( cmb_info->hWnd_static_message, WM_SETFONT, ( WPARAM )cmb_info->hFont_message_box, 0 );
				if ( cmb_info->hWnd_checkbox != NULL )
				{
					_SendMessageW( cmb_info->hWnd_checkbox, WM_SETFONT, ( WPARAM )cmb_info->hFont_message_box, 0 );
				}
				_SendMessageW( cmb_info->hWnd_btn_1, WM_SETFONT, ( WPARAM )cmb_info->hFont_message_box, 0 );
				_SendMessageW( cmb_info->hWnd_btn_2, WM_SETFONT, ( WPARAM )cmb_info->hFont_message_box, 0 );
				_SendMessageW( cmb_info->hWnd_btn_3, WM_SETFONT, ( WPARAM )cmb_info->hFont_message_box, 0 );

				_SetFocus( cmb_info->hWnd_btn_1 );

				// Get the client width and height based on the text dimensions and cmessagebox type.
				RECT cmb_rc;
				CMBGetClientRect( hWnd, cmb_info, &cmb_rc );

				// Disable the parent window.
				_EnableWindow( hWnd_parent, FALSE );

				// Accounts for differing title bar heights.
				CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
				int width = ( cs->cx - ( rc.right - rc.left ) ) + ( cmb_rc.right - cmb_rc.left );
				int height = ( cs->cy - ( rc.bottom - rc.top ) ) + ( cmb_rc.bottom - cmb_rc.top );

				HMONITOR hMon = _MonitorFromWindow( hWnd_parent, MONITOR_DEFAULTTONEAREST );
				MONITORINFO mi;
				mi.cbSize = sizeof( MONITORINFO );
				_GetMonitorInfoW( hMon, &mi );
				_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );


#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					_EnumChildWindows( hWnd, EnumMsgBoxChildProc, NULL );	// Only need the buttons changed.
					_EnumThreadWindows( GetCurrentThreadId(), EnumTLWProc, NULL );
				}
#endif

				return 0;
			}
			else
			{
				return -1;
			}
		}
		break;

		case WM_SIZE:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL )
			{
				RECT rc;
				_GetClientRect( hWnd, &rc );

				HICON hIcon = NULL;

				if ( cmb_info->type & 0xF0 )
				{
					cmb_info->icon_width = _GetSystemMetricsForDpi( SM_CXICON, cmb_info->current_dpi_message_box );
					cmb_info->icon_height = _GetSystemMetricsForDpi( SM_CYICON, cmb_info->current_dpi_message_box );

					LPCWSTR name = NULL;

					switch ( ( cmb_info->type & 0xF0 ) )	// Icon type.
					{
						case CMB_ICONASTERISK:		{ name = MAKEINTRESOURCE( 104 ); } break;	// IDI_ASTERISK
						case CMB_ICONEXCLAMATION:	{ name = MAKEINTRESOURCE( 101 ); } break;	// IDI_EXCLAMATION
						case CMB_ICONQUESTION:		{ name = MAKEINTRESOURCE( 102 ); } break;	// IDI_QUESTION
						case CMB_ICONHAND:			{ name = MAKEINTRESOURCE( 103 ); } break;	// IDI_HAND
					}

					if ( name != NULL )
					{
						hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), name, IMAGE_ICON, cmb_info->icon_width, cmb_info->icon_height, LR_SHARED );
					}
				}

				if ( hIcon != NULL )
				{
					_SendMessageW( cmb_info->hWnd_static_icon, STM_SETICON, ( WPARAM )hIcon, 0 );

					_SetWindowPos( cmb_info->hWnd_static_icon, NULL, _SCALE_MB_( ICON_X_POS ), _SCALE_MB_( ICON_Y_POS ), 0, 0, SWP_NOSIZE );
				}

				RECT cmb_rc;
				CMBGetClientRect( hWnd, cmb_info, &cmb_rc );

				_SetWindowPos( cmb_info->hWnd_static_message, NULL,
							( ( cmb_info->type & 0xF0 ) != 0 ? _SCALE_MB_( ICON_X_POS ) + cmb_info->icon_width + _SCALE_MB_( ICON_PADDING ) : _SCALE_MB_( TEXT_PADDING ) ),
							( ( rc.bottom - rc.top ) - _SCALE_MB_( BTN_AREA_HEIGHT ) - cmb_info->msg_rc.bottom ) / 2,
							cmb_info->msg_rc.right - cmb_info->msg_rc.left,
							cmb_info->msg_rc.bottom - cmb_info->msg_rc.top,
							0 );

				if ( cmb_info->hWnd_checkbox != NULL )
				{
					_SetWindowPos( cmb_info->hWnd_checkbox, NULL, _SCALE_MB_( 10 ), rc.bottom - _SCALE_MB_( 32 ), cmb_info->cb_rc.right - cmb_info->cb_rc.left, _SCALE_MB_( 20 ), 0 );
				}

				if ( ( cmb_info->type & 0x0F ) == CMB_YESNO || ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
				{
					_SetWindowPos( cmb_info->hWnd_btn_1, NULL, rc.right - _SCALE_MB_( 177 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 80 ), _SCALE_MB_( 23 ), 0 );
					_SetWindowPos( cmb_info->hWnd_btn_2, NULL, rc.right - _SCALE_MB_( 89 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 80 ), _SCALE_MB_( 23 ), 0 );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
				{
					_SetWindowPos( cmb_info->hWnd_btn_1, NULL, rc.right - _SCALE_MB_( 280 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
					_SetWindowPos( cmb_info->hWnd_btn_2, NULL, rc.right - _SCALE_MB_( 187 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
					_SetWindowPos( cmb_info->hWnd_btn_3, NULL, rc.right - _SCALE_MB_( 94 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					_SetWindowPos( cmb_info->hWnd_btn_1, NULL, rc.right - _SCALE_MB_( 280 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
					_SetWindowPos( cmb_info->hWnd_btn_2, NULL, rc.right - _SCALE_MB_( 187 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
					_SetWindowPos( cmb_info->hWnd_btn_3, NULL, rc.right - _SCALE_MB_( 94 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 85 ), _SCALE_MB_( 23 ), 0 );
				}
				else	// CMB_OK/ALL or anything that's unsupported.
				{
					_SetWindowPos( cmb_info->hWnd_btn_1, NULL, rc.right - _SCALE_MB_( 89 ), rc.bottom - _SCALE_MB_( 32 ), _SCALE_MB_( 80 ), _SCALE_MB_( 23 ), 0 );
				}
			}

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL )
			{
				return cmb_info->current_dpi_message_box;
			}
			else
			{
				return 0;
			}
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = 0;

			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL )
			{
				last_dpi = cmb_info->current_dpi_message_box;
				cmb_info->current_dpi_message_box = HIWORD( wParam );

				HFONT hFont = UpdateFont( cmb_info->current_dpi_message_box );
				EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
				_DeleteObject( cmb_info->hFont_message_box );
				cmb_info->hFont_message_box = hFont;
			}

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( cmb_info->last_dpi_message_box == 0 )
			{
				HMONITOR hMon = _MonitorFromWindow( _GetParent( hWnd ), MONITOR_DEFAULTTONEAREST );
				MONITORINFO mi;
				mi.cbSize = sizeof( MONITORINFO );
				_GetMonitorInfoW( hMon, &mi );
				_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );
			}
			else
			{
				_SetWindowPos( hWnd, NULL, rc->left, rc->top, width, height, SWP_NOZORDER | SWP_NOACTIVATE );
			}

			cmb_info->last_dpi_message_box = last_dpi;

			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL && cmb_info->use_theme )
			{
				if ( ( HWND )lParam != cmb_info->hWnd_checkbox )
				{
					return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
				}
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_PAINT:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL && cmb_info->use_theme )
			{
				PAINTSTRUCT ps;
				HDC hDC = _BeginPaint( hWnd, &ps );

				RECT client_rc;
				_GetClientRect( hWnd, &client_rc );
				client_rc.bottom -= _SCALE_MB_( BTN_AREA_HEIGHT );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );
				_DeleteObject( hbm );

				HBRUSH color;

#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					color = _CreateSolidBrush( dm_color_edit_background );
				}
				else
#endif
				{
					color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
				}

				// Fill the background.
				_FillRect( hdcMem, &client_rc, color );
				_DeleteObject( color );

				// Draw our memory buffer to the main device context.
				_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

				// Delete our back buffer.
				_DeleteDC( hdcMem );
				_EndPaint( hWnd, &ps );

				return 0;
			}
			else
			{
				return _DefWindowProcW( hWnd, msg, wParam, lParam );
			}
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_YES:
				case BTN_OK:
				case BTN_RENAME:
				case BTN_CONTINUE:
				{
					CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
					if ( cmb_info != NULL )
					{
						if ( ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDYESALL : CMBIDYES ), 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_YESNO )
						{
							_SendMessageW( hWnd, WM_CLOSE, CMBIDYES, 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDRENAMEALL : CMBIDRENAME ), 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDCONTINUEALL : CMBIDCONTINUE ), 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_OKALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDOKALL : CMBIDOK ), 0 );
						}
						else
						{
							_SendMessageW( hWnd, WM_CLOSE, CMBIDOK, 0 );
						}
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, CMBIDFAIL, 0 );
					}
				}
				break;

				case BTN_NO:
				case BTN_SKIP:
				{
					CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
					if ( cmb_info != NULL )
					{
						if ( ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDNOALL : CMBIDNO ), 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_YESNO )
						{
							_SendMessageW( hWnd, WM_CLOSE, CMBIDNO, 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
								  ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDSKIPALL : CMBIDSKIP ), 0 );
						}
						else
						{
							_SendMessageW( hWnd, WM_CLOSE, CMBIDFAIL, 0 );
						}
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, CMBIDFAIL, 0 );
					}
				}
				break;

				case BTN_OVERWRITE:
				case BTN_RESTART:
				{
					CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
					if ( cmb_info != NULL )
					{
						if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDOVERWRITEALL : CMBIDOVERWRITE ), 0 );
						}
						else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
						{
							_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cmb_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDRESTARTALL : CMBIDRESTART ), 0 );
						}
						else
						{
							_SendMessageW( hWnd, WM_CLOSE, CMBIDFAIL, 0 );
						}
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, CMBIDFAIL, 0 );
					}
				}
				break;
			}

			return 0;
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
			// Enable the parent window.
			_EnableWindow( _GetParent( hWnd ), TRUE );

			WPARAM ret_type = CMBIDOK;

			if ( wParam == 0 )
			{
				CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
				if ( cmb_info != NULL )
				{
					if ( ( cmb_info->type & 0x0F ) == CMB_YESNO ||
						 ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
					{
						ret_type = CMBIDNO;
					}
					else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
							  ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
					{
						ret_type = CMBIDSKIP;
					}
				}
			}
			else
			{
				ret_type = wParam;
			}

			// Post a message to our message loop to trigger it to destroy the window and exit the loop.
			_PostMessageW( hWnd, WM_DESTROY_CMSGBOX, ret_type, lParam );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cmb_info != NULL )
			{
				// Delete our font.
				_DeleteObject( cmb_info->hFont_message_box );

				GlobalFree( cmb_info->cmsgbox_message );
				GlobalFree( cmb_info );
			}

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
