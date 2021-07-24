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

HWND g_hWnd_btn_yes = NULL;
HWND g_hWnd_btn_no = NULL;
HWND g_hWnd_btn_ok = NULL;
HWND g_hWnd_btn_rename = NULL;
HWND g_hWnd_btn_overwrite = NULL;
HWND g_hWnd_btn_continue = NULL;
HWND g_hWnd_btn_restart = NULL;
HWND g_hWnd_btn_skip = NULL;

#define BTN_AREA_HEIGHT			42

#define ICON_X_POS				21
#define ICON_Y_POS				23

#define ICON_WIDTH				32
#define ICON_HEIGHT				32

#define ICON_PADDING			10
#define TEXT_PADDING			10

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
	CMSGBOX_INFO *cmb_info = ( CMSGBOX_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CMSGBOX_INFO ) );
	cmb_info->cmsgbox_message = GlobalStrDupW( lpText );
	cmb_info->type = uType;
	cmb_info->hWnd_checkbox = NULL;

	bool use_theme = true;

	#ifndef UXTHEME_USE_STATIC_LIB
	if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
	{
		use_theme = InitializeUXTheme();
	}
	#endif

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

		// Pass our messagebox info to the window.
		_SetWindowLongPtrW( hWnd_cmsgbox, 0, ( LONG_PTR )cmb_info );

		// Force the window to be painted.
		_ShowWindow( hWnd_cmsgbox, SW_SHOW );

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

	return ret;
}

int CPromptW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, void *data )
{
	int ret = 0;

	// This struct is created for each instance of a messagebox window.
	CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CPROMPT_INFO ) );
	cp_info->cp_message = GlobalStrDupW( lpText );
	cp_info->type = uType;
	cp_info->hWnd_checkbox = NULL;
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

		// Pass our messagebox info to the window.
		_SetWindowLongPtrW( hWnd_cmsgbox, 0, ( LONG_PTR )cp_info );

		// Force the window to be painted.
		_ShowWindow( hWnd_cmsgbox, SW_SHOW );

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

	return ret;
}

// Width and height are hardcoded to match a messagebox window with 8 point (13 px) Tahoma font.
void AdjustWindowDimensions( HWND hWnd, HWND static_msg, CMSGBOX_INFO *cmb_info, wchar_t *cb_message )
{
	// Limit the width of the text area that's being drawn.

	RECT text_rc, cb_text_rc;
	_memzero( &text_rc, sizeof( RECT ) );
	_memzero( &cb_text_rc, sizeof( RECT ) );

	//RECT wa;
	//_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );
	HMONITOR hMon = _MonitorFromWindow( _GetParent( hWnd ), MONITOR_DEFAULTTONEAREST );
	MONITORINFO mi;
	mi.cbSize = sizeof( MONITORINFO );
	_GetMonitorInfoW( hMon, &mi );

	TEXTMETRIC tm;

	HDC hDC = _GetDC( hWnd );
	HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
	_GetTextMetricsW( hDC, &tm );
	_DeleteObject( ohf );

	// Calculate the height and width of the message.
	if ( cmb_info->cmsgbox_message != NULL )
	{
		text_rc.right = mi.rcWork.right - mi.rcWork.left;
		text_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &text_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
	}

	int cb_width;

	if ( cb_message != NULL )
	{
		cb_text_rc.right = mi.rcWork.right - mi.rcWork.left;
		cb_text_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cb_message, -1, &cb_text_rc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE );

		cb_width = ( cb_text_rc.right - cb_text_rc.left ) + MulDiv( 15, tm.tmAveCharWidth, 4 );	// Add padding for checkbox glyph.
	}
	else
	{
		cb_width = 0;
	}

	int msgbox_width;

	if ( cmb_info->type & 0xF0 )	// Icon.
	{
		msgbox_width = ICON_X_POS + ICON_WIDTH + ICON_PADDING + ( text_rc.right - text_rc.left ) + ICON_X_POS;
	}
	else
	{
		msgbox_width = TEXT_PADDING + ( text_rc.right - text_rc.left ) + TEXT_PADDING;
	}

	int twidth = MulDiv( 278, max( tm.tmAveCharWidth, 6 ), 4 );	// DLU to px.
	if ( twidth < msgbox_width ) { msgbox_width = twidth; }

	// Minimum widths for the windows.
	if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
		 ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
	{
		if ( msgbox_width < ( 283 + cb_width ) )
		{
			msgbox_width = 283 + cb_width;	// ( 75 + 8 + 75 + 8 + 75 + 10 ) + 32
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
	{
		if ( msgbox_width < ( 200 + cb_width ) )
		{
			msgbox_width = 200 + cb_width;
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_YESNO )
	{
		if ( msgbox_width < 200 )
		{
			msgbox_width = 200;	// ( 75 + 8 + 75 + 10 ) + 32
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_OKALL )
	{
		if ( msgbox_width < ( 117 + cb_width ) )
		{
			msgbox_width = 117 + cb_width;
		}
	}
	else	// CMB_OK or anything that's unsupported.
	{
		if ( msgbox_width < 117 )
		{
			msgbox_width = 117;	// ( 75 + 10 ) + 32
		}
	}

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
		text_rc.right = msgbox_width - ( ICON_X_POS + ICON_WIDTH + ICON_PADDING + ICON_X_POS );
	}
	else
	{
		text_rc.right = msgbox_width - ( TEXT_PADDING + TEXT_PADDING );
	}

	// Recalculate the text message area based on the new message box width.
	if ( cmb_info->cmsgbox_message != NULL )
	{
		text_rc.bottom = mi.rcWork.bottom - mi.rcWork.top;

		_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &text_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
	}
	_ReleaseDC( hWnd, hDC );

	LONG msgbox_height = ICON_Y_POS + max( ( text_rc.bottom - text_rc.top ), ICON_HEIGHT ) + ICON_Y_POS + BTN_AREA_HEIGHT;

	// We need dimensions to calculate the non-client difference.
	_SetWindowPos( hWnd, NULL, 0, 0, msgbox_width, msgbox_height, 0 );

	RECT rc, rc2;
	_GetWindowRect( hWnd, &rc );
	_GetClientRect( hWnd, &rc2 );

	msgbox_width += ( rc.right - rc.left - rc2.right );
	msgbox_height += ( rc.bottom - rc.top - rc2.bottom );

	// Center the window on screen.
	_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - msgbox_width ) / 2 ),
							   mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - msgbox_height ) / 2 ),
							   msgbox_width,
							   msgbox_height,
							   0 );

	_GetClientRect( hWnd, &rc );
	// Position the message text.
	_SetWindowPos( static_msg, NULL, ( ( cmb_info->type & 0xF0 ) != 0 ? ICON_X_POS + ICON_WIDTH + ICON_PADDING : TEXT_PADDING ),
									 ( ( rc.bottom - rc.top ) - BTN_AREA_HEIGHT - ( text_rc.bottom - text_rc.top ) ) / 2,
									 text_rc.right - text_rc.left,
									 text_rc.bottom - text_rc.top,
									 0 );

	if ( cmb_info->hWnd_checkbox != NULL )
	{
		_SetWindowPos( cmb_info->hWnd_checkbox, NULL, 10, rc.bottom - 32, cb_width, 23, 0 );
	}
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
				HWND hWnd_static_icon = _CreateWindowW( WC_STATIC, NULL, SS_ICON | WS_CHILD | ( ( cmb_info->type & 0xF0 ) != 0 ? WS_VISIBLE : 0 ), ICON_X_POS, ICON_Y_POS, 0, 0, hWnd, NULL, NULL, NULL );

				bool play = true;
				#ifndef WINMM_USE_STATIC_LIB
					if ( winmm_state == WINMM_STATE_SHUTDOWN )
					{
						play = InitializeWinMM();
					}
				#endif

				switch ( ( cmb_info->type & 0xF0 ) )	// Icon type.
				{
					case CMB_ICONASTERISK:
					{
						_SendMessageW( hWnd_static_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_ASTERISK ), 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMASTERISK, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}
					break;

					case CMB_ICONEXCLAMATION:
					{
						_SendMessageW( hWnd_static_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_EXCLAMATION ), 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMEXCLAMATION, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}
					break;

					case CMB_ICONQUESTION:
					{
						_SendMessageW( hWnd_static_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_QUESTION ), 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMQUESTION, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}
					break;

					case CMB_ICONHAND:
					{
						_SendMessageW( hWnd_static_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_HAND ), 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMHAND, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}
					break;
				}

				HWND hWnd_static_message = _CreateWindowW( WC_STATIC, cmb_info->cmsgbox_message, SS_EDITCONTROL | SS_LEFT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

				_SendMessageW( hWnd_static_message, WM_SETFONT, ( WPARAM )g_hFont, 0 );

				wchar_t *cb_message;
				if ( ( cmb_info->type & 0x0F ) == CMB_OKALL ||
					 ( cmb_info->type & 0x0F ) == CMB_YESNOALL ||
					 ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
					 ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					cb_message = ( ( cmb_info->type & 0x0F ) == CMB_OKALL ? ST_V_Skip_remaining_messages : ST_V_Remember_choice );

					cmb_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, cb_message, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
					_SendMessageW( cmb_info->hWnd_checkbox, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}
				else
				{
					cb_message = NULL;
				}

				// This will resize the window and static control based on the text dimensions and cmessagebox type.
				AdjustWindowDimensions( hWnd, hWnd_static_message, cmb_info, cb_message );

				RECT rc;
				_GetClientRect( hWnd, &rc );

				if ( ( cmb_info->type & 0x0F ) == CMB_YESNO || ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
				{
					g_hWnd_btn_yes = _CreateWindowW( WC_BUTTON, ST_V_Yes, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 167, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_YES, NULL, NULL );
					g_hWnd_btn_no = _CreateWindowW( WC_BUTTON, ST_V_No, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 84, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_NO, NULL, NULL );

					_SendMessageW( g_hWnd_btn_yes, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_no, WM_SETFONT, ( WPARAM )g_hFont, 0 );

					_SetFocus( g_hWnd_btn_yes );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
				{
					g_hWnd_btn_rename = _CreateWindowW( WC_BUTTON, ST_V_Rename, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 250, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_RENAME, NULL, NULL );
					g_hWnd_btn_overwrite = _CreateWindowW( WC_BUTTON, ST_V_Overwrite, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 167, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_OVERWRITE, NULL, NULL );
					g_hWnd_btn_skip = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 84, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );

					_SendMessageW( g_hWnd_btn_rename, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_overwrite, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_skip, WM_SETFONT, ( WPARAM )g_hFont, 0 );

					_SetFocus( g_hWnd_btn_rename );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					g_hWnd_btn_continue = _CreateWindowW( WC_BUTTON, ST_V_Continue, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 250, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_CONTINUE, NULL, NULL );
					g_hWnd_btn_restart = _CreateWindowW( WC_BUTTON, ST_V_Restart, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 167, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_RESTART, NULL, NULL );
					g_hWnd_btn_skip = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 84, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );

					_SendMessageW( g_hWnd_btn_continue, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_restart, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_skip, WM_SETFONT, ( WPARAM )g_hFont, 0 );

					_SetFocus( g_hWnd_btn_continue );
				}
				else	// CMB_OK/ALL or anything that's unsupported.
				{
					g_hWnd_btn_ok = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 84, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );

					_SendMessageW( g_hWnd_btn_ok, WM_SETFONT, ( WPARAM )g_hFont, 0 );

					_SetFocus( g_hWnd_btn_ok );
				}

				// Disable the parent window.
				_EnableWindow( _GetParent( hWnd ), FALSE );

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
				client_rc.bottom -= BTN_AREA_HEIGHT;

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
				GlobalFree( cmb_info->cmsgbox_message );
				GlobalFree( cmb_info );
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
	//return TRUE;
}
