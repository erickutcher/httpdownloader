/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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

#include "menus.h"
#include "string_tables.h"
#include "cmessagebox.h"

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

bool InitializeCMessageBox( HINSTANCE hInstance )
{
	// Set/Get won't work after a window has been created. Have to register separate classes.
	//_SetClassLongPtrW( hWnd, GCL_STYLE, _GetClassLongPtrW( hWnd, GCL_STYLE ) | CS_NOCLOSE );
	//_SetClassLongPtrW( hWnd, GCL_STYLE, _GetClassLongPtrW( hWnd, GCL_STYLE ) & ~CS_NOCLOSE );

	// Initialize our window class.
	WNDCLASSEX wcex;
	_memzero( &wcex, sizeof( WNDCLASSEX ) );
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style			= CS_VREDRAW | CS_HREDRAW;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hCursor		= _LoadCursorW( NULL, IDC_ARROW );
	wcex.hbrBackground	= ( HBRUSH )( COLOR_WINDOW );
	wcex.cbWndExtra		= sizeof( LONG_PTR );		// We're going to pass each message window an info struct pointer.
	wcex.lpfnWndProc	= CustomMessageBoxWndProc;

	wcex.lpszClassName	= L"cmessagebox";
	if ( !_RegisterClassExW( &wcex ) )
	{
		return false;
	}

	// Disable the close button.
	wcex.style		   |= CS_NOCLOSE;
	wcex.lpszClassName	= L"cmessageboxdc";
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

	HWND hWnd_cmsgbox = _CreateWindowExW( WS_EX_DLGMODALFRAME, ( ( ( uType & 0x0F ) == CMB_YESNO ||
																   ( uType & 0x0F ) == CMB_YESNOALL ||
																   ( uType & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
																   ( uType & 0x0F ) == CMB_CONTINUERESTARTSKIPALL ||
																   ( uType & 0x0F ) == CMB_OKALL ) ? L"cmessageboxdc" : L"cmessagebox" ), lpCaption, WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, 0, 0, 0, 0, hWnd, NULL, NULL, ( LPVOID )cmb_info );
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

// Width and height are hardcoded to match a messagebox window with 8 point (13 px) Tahoma font.
void AdjustWindowDimensions( HWND hWnd, HWND static_msg, CMSGBOX_INFO *cmb_info )
{
	// Limit the width of the text area that's being drawn.

	RECT text_rc;
	_memzero( &text_rc, sizeof( text_rc ) );

	int screen_width = _GetSystemMetrics( SM_CXSCREEN );

	text_rc.right = screen_width;

	TEXTMETRIC tm;

	HDC hDC = _GetDC( hWnd );
	HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
	_GetTextMetricsW( hDC, &tm );
	_DeleteObject( ohf );

	int font_height = tm.tmHeight + tm.tmExternalLeading;

	// Calculate the height and width of the message.
	_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &text_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
	_ReleaseDC( hWnd, hDC );

	LONG width = text_rc.right - text_rc.left;
	LONG height = text_rc.bottom - text_rc.top;

	// The scale divisor should be based on the size of the font.
	// The larger the font, the smaller the divisor.
	// I use 8pt Tahoma on my system and 6 matches the dimensions of the standard message box.
	// Ideally the dimensions and padding should be based on a factor, but I have no interest in matching the look of the message box 100%.
	int max_width = screen_width / 6;

	if ( width > max_width )
	{
		height = height * width / max_width;
		width = max_width - 1;

		if ( !( cmb_info->type & 0xF0 ) )	// No icon.
		{
			width += 48;
		}

		text_rc.left = 0;
		text_rc.right = width;
		text_rc.top = 0;
		text_rc.bottom = height;

		hDC = _GetDC( hWnd );
		ohf = ( HFONT )_SelectObject( hDC, g_hFont );
		_DeleteObject( ohf );

		// Calculate the height and width of the message.
		_DrawTextW( hDC, cmb_info->cmsgbox_message, -1, &text_rc, DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL | DT_WORDBREAK );
		_ReleaseDC( hWnd, hDC );

		width = text_rc.right - text_rc.left;

		if ( height == font_height && ( cmb_info->type & 0xF0 ) )	// One line of text and display icon.
		{
			height = ( text_rc.bottom - text_rc.top ) + 6;
		}
		else														// Multiple lines and no icon.
		{
			height = text_rc.bottom - text_rc.top;
		}

		if ( ( cmb_info->type & 0xF0 ) )		// Display icon.
		{
			width += 99;
		}
		else									// No icon.
		{
			width += 48;
		}
	}
	else
	{
		if ( ( cmb_info->type & 0xF0 ) )		// Display icon.
		{
			if ( height < font_height )			// No text.
			{
				height += 32;
			}
			else if ( height == font_height )	// One line of text.
			{
				height += 18;
			}

			width += 96;
		}
		else									// No icon.
		{
			width += 64;
		}
	}

	height += 114;

	// Minimum widths for the windows.
	if ( ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
	{
		if ( width < 310 )
		{
			width = 310;
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_YESNO )
	{
		if ( width < 206 )
		{
			width = 206;
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
			  ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
	{
		if ( width < 390 )
		{
			width = 390;
		}
	}
	else if ( ( cmb_info->type & 0x0F ) == CMB_OKALL )
	{
		if ( width < 260 )
		{
			width = 260;
		}
	}
	else	// CMB_OK or anything that's unsupported.
	{
		if ( width < 128 )
		{
			width = 128;
		}
	}

	// Center the window on screen.
	_SetWindowPos( hWnd, NULL, ( ( _GetSystemMetrics( SM_CXSCREEN ) - width ) / 2 ),
							   ( ( _GetSystemMetrics( SM_CYSCREEN ) - height ) / 2 ),
							   width,
							   height,
							   0 );

	// Get the new dimensions of the window.
	RECT rc;
	_GetClientRect( hWnd, &rc );

	// Position the static text.
	_SetWindowPos( static_msg, NULL, ( ( cmb_info->type & 0xF0 ) != 0 ? 62 : 11 ),
									 ( ( ( ( rc.bottom - 42 ) - rc.top ) - ( text_rc.bottom - text_rc.top ) ) / 2 ),
									 text_rc.right - text_rc.left,
									 text_rc.bottom - text_rc.top,
									 0 );
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
				HWND hWnd_static_icon = _CreateWindowW( WC_STATIC, NULL, SS_ICON | WS_CHILD | ( ( cmb_info->type & 0xF0 ) != 0 ? WS_VISIBLE : 0 ), 21, 23, 0, 0, hWnd, NULL, NULL, NULL );

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

				// This will resize the window and static control based on the text dimensions and cmessagebox type.
				AdjustWindowDimensions( hWnd, hWnd_static_message, cmb_info );

				RECT rc;
				_GetClientRect( hWnd, &rc );

				if ( ( cmb_info->type & 0x0F ) == CMB_OKALL ||
					 ( cmb_info->type & 0x0F ) == CMB_YESNOALL ||
					 ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL ||
					 ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					if ( ( cmb_info->type & 0x0F ) == CMB_OKALL )
					{
						cmb_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, ST_V_Skip_remaining_messages, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, rc.bottom - 32, 150, 23, hWnd, NULL, NULL, NULL );
					}
					else
					{
						cmb_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, ST_V_Remember_choice, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, rc.bottom - 32, 120, 23, hWnd, NULL, NULL, NULL );
					}

					_SendMessageW( cmb_info->hWnd_checkbox, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}

				if ( ( cmb_info->type & 0x0F ) == CMB_YESNO || ( cmb_info->type & 0x0F ) == CMB_YESNOALL )
				{
					g_hWnd_btn_yes = _CreateWindowW( WC_BUTTON, ST_V_Yes, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 165, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_YES, NULL, NULL );
					g_hWnd_btn_no = _CreateWindowW( WC_BUTTON, ST_V_No, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 83, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_NO, NULL, NULL );

					_SendMessageW( g_hWnd_btn_yes, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_no, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_RENAMEOVERWRITESKIPALL )
				{
					g_hWnd_btn_rename = _CreateWindowW( WC_BUTTON, ST_V_Rename, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 247, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_RENAME, NULL, NULL );
					g_hWnd_btn_overwrite = _CreateWindowW( WC_BUTTON, ST_V_Overwrite, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 165, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_OVERWRITE, NULL, NULL );
					g_hWnd_btn_skip = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 83, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );

					_SendMessageW( g_hWnd_btn_rename, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_overwrite, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_skip, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}
				else if ( ( cmb_info->type & 0x0F ) == CMB_CONTINUERESTARTSKIPALL )
				{
					g_hWnd_btn_continue = _CreateWindowW( WC_BUTTON, ST_V_Continue, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 247, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_CONTINUE, NULL, NULL );
					g_hWnd_btn_restart = _CreateWindowW( WC_BUTTON, ST_V_Restart, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 165, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_RESTART, NULL, NULL );
					g_hWnd_btn_skip = _CreateWindowW( WC_BUTTON, ST_V_Skip, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 83, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_SKIP, NULL, NULL );

					_SendMessageW( g_hWnd_btn_continue, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_restart, WM_SETFONT, ( WPARAM )g_hFont, 0 );
					_SendMessageW( g_hWnd_btn_skip, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}
				else	// CMB_OK/ALL or anything that's unsupported.
				{
					g_hWnd_btn_ok = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 83, rc.bottom - 32, 75, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );

					_SendMessageW( g_hWnd_btn_ok, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				}

				// Disable the parent window.
				_EnableWindow( _GetParent( hWnd ), FALSE );

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
			if ( cmb_info != NULL )
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
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc;
			_GetClientRect( hWnd, &client_rc );
			client_rc.bottom -= 42;

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
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

			// Post a message to our message loop to trigger it to destroy the window and exit the loop.
			_PostMessageW( hWnd, WM_DESTROY_CMSGBOX, wParam, lParam );

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

	return TRUE;
}
