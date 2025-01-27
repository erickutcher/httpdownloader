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
#include "lite_winmm.h"
#include "lite_gdi32.h"
#include "string_tables.h"
#include "utilities.h"
#include "sftp.h"
#include "cmessagebox.h"

#include "dark_mode.h"

#define FINGERPRINT_PROMPT_WINDOW_WIDTH		600
#define FINGERPRINT_PROMPT_CLIENT_HEIGHT	223

#define BTN_FP_YES				1000
#define BTN_FP_NO				1001

WNDPROC FingerprintsProc = NULL;

#define _SCALE_FP_( x )						MulDiv( ( x ), cp_info->current_dpi_fingerprint_prompt, USER_DEFAULT_SCREEN_DPI )

LRESULT CALLBACK FingerprintsSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_GETDLGCODE:
		{
			// Don't process the tab key in the edit control. (Allows us to tab between controls)
			if ( wParam == VK_TAB )
			{
				return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
			}
		}
		break;
	}

	return _CallWindowProcW( FingerprintsProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK FingerprintPromptWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )( ( CREATESTRUCT * )lParam )->lpCreateParams;
			if ( cp_info != NULL )
			{
				// Pass our messagebox info to the window.
				_SetWindowLongPtrW( hWnd, 0, ( LONG_PTR )cp_info );

				if ( cp_info->data != NULL )
				{
					KEY_PROMPT_INFO *kpi = ( KEY_PROMPT_INFO * )cp_info->data;

					HWND hWnd_parent = _GetParent( hWnd );

					cp_info->current_dpi_fingerprint_prompt = __GetDpiForWindow( hWnd );
					cp_info->last_dpi_fingerprint_prompt = ( cp_info->current_dpi_fingerprint_prompt == ( UINT )_SendMessageW( hWnd_parent, WM_GET_DPI, 0, 0 ) ? cp_info->current_dpi_fingerprint_prompt : 0 );
					cp_info->hFont_fingerprint_prompt = UpdateFont( cp_info->current_dpi_fingerprint_prompt );

					RECT rc, rc_icon;
					_GetClientRect( hWnd, &rc );

					cp_info->hWnd_static_fp_icon = _CreateWindowW( WC_STATIC, NULL, SS_ICON | WS_CHILD | WS_VISIBLE, 10, 10, 0, 0, hWnd, NULL, NULL, NULL );

					bool play = true;
					#ifndef WINMM_USE_STATIC_LIB
						if ( winmm_state == WINMM_STATE_SHUTDOWN )
						{
							play = InitializeWinMM();
						}
					#endif

					if ( ( cp_info->type & 0xF0 ) == CMB_ICONQUESTION )
					{
						// IDI_QUESTION
						HICON hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), MAKEINTRESOURCE( 102 ), IMAGE_ICON, __GetSystemMetricsForDpi( SM_CXICON, cp_info->current_dpi_fingerprint_prompt ), __GetSystemMetricsForDpi( SM_CYICON, cp_info->current_dpi_fingerprint_prompt ), LR_SHARED );
						_SendMessageW( cp_info->hWnd_static_fp_icon, STM_SETICON, ( WPARAM )hIcon, 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMQUESTION, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}
					else if ( ( cp_info->type & 0xF0 ) == CMB_ICONHAND )
					{
						// IDI_HAND
						HICON hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), MAKEINTRESOURCE( 103 ), IMAGE_ICON, __GetSystemMetricsForDpi( SM_CXICON, cp_info->current_dpi_fingerprint_prompt ), __GetSystemMetricsForDpi( SM_CYICON, cp_info->current_dpi_fingerprint_prompt ), LR_SHARED );
						_SendMessageW( cp_info->hWnd_static_fp_icon, STM_SETICON, ( WPARAM )hIcon, 0 );

						if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMHAND, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
					}

					_GetClientRect( cp_info->hWnd_static_fp_icon, &rc_icon );

					cp_info->hWnd_static_fp_msg = _CreateWindowW( WC_STATIC, cp_info->cp_message, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 10, rc.right - ( rc_icon.right * 2 ), 35, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_static_fp_host = _CreateWindowW( WC_STATIC, ST_V_Host_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 53, 110, 15, hWnd, NULL, NULL, NULL );
					cp_info->hWnd_host = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_host, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 50, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_static_fp_key_algorithm = _CreateWindowW( WC_STATIC, ST_V_Host_key_algorithm_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 81, 110, 15, hWnd, NULL, NULL, NULL );
					cp_info->hWnd_key_algorithm = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_algorithm, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 78, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_static_fp_key_size = _CreateWindowW( WC_STATIC, ST_V_Key_size_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 109, 110, 15, hWnd, NULL, NULL, NULL );
					cp_info->hWnd_key_size = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_size, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 106, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_static_fp_key_fingerprints = _CreateWindowW( WC_STATIC, ST_V_Fingerprints_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 137, 110, 15, hWnd, NULL, NULL, NULL );
					cp_info->hWnd_key_fingerprints = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_fingerprints, ES_AUTOHSCROLL | ES_READONLY | ES_MULTILINE | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 134, rc.right - 155 - rc_icon.right, 46, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, ST_V_Add_host_and_key_information_to_cache, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, 190, rc.right - 190, 20, hWnd, NULL, NULL, NULL );

					cp_info->hWnd_fp_yes = _CreateWindowW( WC_BUTTON, ST_V_Yes, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, 190, 80, 23, hWnd, ( HMENU )BTN_FP_YES, NULL, NULL );
					cp_info->hWnd_fp_no = _CreateWindowW( WC_BUTTON, ST_V_No, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 90, 190, 80, 23, hWnd, ( HMENU )BTN_FP_NO, NULL, NULL );

					FingerprintsProc = ( WNDPROC )_GetWindowLongPtrW( cp_info->hWnd_key_fingerprints, GWLP_WNDPROC );
					_SetWindowLongPtrW( cp_info->hWnd_key_fingerprints, GWLP_WNDPROC, ( LONG_PTR )FingerprintsSubProc );

					_SendMessageW( cp_info->hWnd_static_fp_msg, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_static_fp_host, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_host, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_static_fp_key_algorithm, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_key_algorithm, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_static_fp_key_size, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_key_size, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_static_fp_key_fingerprints, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_key_fingerprints, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_checkbox, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_fp_yes, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );
					_SendMessageW( cp_info->hWnd_fp_no, WM_SETFONT, ( WPARAM )cp_info->hFont_fingerprint_prompt, 0 );

					// Disable the parent window.
					_EnableWindow( hWnd_parent, FALSE );

					int width = _SCALE_FP_( FINGERPRINT_PROMPT_WINDOW_WIDTH );

					// Accounts for differing title bar heights.
					CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
					int height = ( cs->cy - ( rc.bottom - rc.top ) ) + _SCALE_FP_( FINGERPRINT_PROMPT_CLIENT_HEIGHT );	// Bottom of last window object + 10.

					HMONITOR hMon = _MonitorFromWindow( hWnd_parent, MONITOR_DEFAULTTONEAREST );
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
			}

			return -1;
		}
		break;

		case WM_SIZE:
		{
			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cp_info != NULL )
			{
				RECT rc, rc_icon;
				_GetClientRect( hWnd, &rc );

				if ( ( cp_info->type & 0xF0 ) == CMB_ICONQUESTION )
				{
					// IDI_QUESTION
					HICON hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), MAKEINTRESOURCE( 102 ), IMAGE_ICON, __GetSystemMetricsForDpi( SM_CXICON, cp_info->current_dpi_fingerprint_prompt ), __GetSystemMetricsForDpi( SM_CYICON, cp_info->current_dpi_fingerprint_prompt ), LR_SHARED );
					_SendMessageW( cp_info->hWnd_static_fp_icon, STM_SETICON, ( WPARAM )hIcon, 0 );
				}
				else if ( ( cp_info->type & 0xF0 ) == CMB_ICONHAND )
				{
					// IDI_HAND
					HICON hIcon = ( HICON )_LoadImageW( GetModuleHandleW( L"user32.dll" ), MAKEINTRESOURCE( 103 ), IMAGE_ICON, __GetSystemMetricsForDpi( SM_CXICON, cp_info->current_dpi_fingerprint_prompt ), __GetSystemMetricsForDpi( SM_CYICON, cp_info->current_dpi_fingerprint_prompt ), LR_SHARED );
					_SendMessageW( cp_info->hWnd_static_fp_icon, STM_SETICON, ( WPARAM )hIcon, 0 );
				}

				_GetClientRect( cp_info->hWnd_static_fp_icon, &rc_icon );

				HDWP hdwp = _BeginDeferWindowPos( 13 );
				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_icon, HWND_TOP, _SCALE_FP_( 10 ), _SCALE_FP_( 10 ), rc_icon.right - rc_icon.left, rc_icon.bottom - rc_icon.top, SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_msg, HWND_TOP, rc_icon.right + _SCALE_FP_( 20 ), _SCALE_FP_( 10 ), rc.right - ( rc_icon.right * 2 ), _SCALE_FP_( 35 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_host, HWND_TOP, rc_icon.right + _SCALE_FP_( 20 ), _SCALE_FP_( 53 ), _SCALE_FP_( 110 ), _SCALE_FP_( 17 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_host, HWND_TOP, rc_icon.right + _SCALE_FP_( 145 ), _SCALE_FP_( 50 ), rc.right - _SCALE_FP_( 155 ) - rc_icon.right, _SCALE_FP_( 23 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_key_algorithm, HWND_TOP, rc_icon.right + _SCALE_FP_( 20 ), _SCALE_FP_( 81 ), _SCALE_FP_( 110 ), _SCALE_FP_( 17 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_key_algorithm, HWND_TOP, rc_icon.right + _SCALE_FP_( 145 ), _SCALE_FP_( 78 ), rc.right - _SCALE_FP_( 155 ) - rc_icon.right, _SCALE_FP_( 23 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_key_size, HWND_TOP, rc_icon.right + _SCALE_FP_( 20 ), _SCALE_FP_( 109 ), _SCALE_FP_( 110 ), _SCALE_FP_( 17 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_key_size, HWND_TOP, rc_icon.right + _SCALE_FP_( 145 ), _SCALE_FP_( 106 ), rc.right - _SCALE_FP_( 155 ) - rc_icon.right, _SCALE_FP_( 23 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_static_fp_key_fingerprints, HWND_TOP, rc_icon.right + _SCALE_FP_( 20 ), _SCALE_FP_( 137 ), _SCALE_FP_( 110 ), _SCALE_FP_( 17 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_key_fingerprints, HWND_TOP, rc_icon.right + _SCALE_FP_( 145 ), _SCALE_FP_( 134 ), rc.right - _SCALE_FP_( 155 ) - rc_icon.right, _SCALE_FP_( 46 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_checkbox, HWND_TOP, _SCALE_FP_( 10 ), _SCALE_FP_( 190 ), rc.right - _SCALE_FP_( 190 ), _SCALE_FP_( 20 ), SWP_NOZORDER );

				_DeferWindowPos( hdwp, cp_info->hWnd_fp_yes, HWND_TOP, rc.right - _SCALE_FP_( 175 ), _SCALE_FP_( 190 ), _SCALE_FP_( 80 ), _SCALE_FP_( 23 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, cp_info->hWnd_fp_no, HWND_TOP, rc.right - _SCALE_FP_( 90 ), _SCALE_FP_( 190 ), _SCALE_FP_( 80 ), _SCALE_FP_( 23 ), SWP_NOZORDER );

				_EndDeferWindowPos( hdwp );
			}

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cp_info != NULL )
			{
				return cp_info->current_dpi_fingerprint_prompt;
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

			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cp_info != NULL )
			{
				last_dpi = cp_info->current_dpi_fingerprint_prompt;
				cp_info->current_dpi_fingerprint_prompt = HIWORD( wParam );

				HFONT hFont = UpdateFont( cp_info->current_dpi_fingerprint_prompt );
				EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
				_DeleteObject( cp_info->hFont_fingerprint_prompt );
				cp_info->hFont_fingerprint_prompt = hFont;
			}

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( cp_info->last_dpi_fingerprint_prompt == 0 )
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

			cp_info->last_dpi_fingerprint_prompt = last_dpi;

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_FP_YES:
				{
					CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )_GetWindowLongPtrW( hWnd, 0 );
					if ( cp_info != NULL )
					{
						_SendMessageW( hWnd, WM_CLOSE, ( _SendMessageW( cp_info->hWnd_checkbox, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? CMBIDYESADD : CMBIDYES ), 0 );
					}
				}
				break;

				case BTN_FP_NO:
				{
					_SendMessageW( hWnd, WM_CLOSE, CMBIDNO, 0 );
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
			_PostMessageW( hWnd, WM_DESTROY_CMSGBOX, ( wParam == 0 ? CMBIDNO : wParam ), lParam );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )_GetWindowLongPtrW( hWnd, 0 );
			if ( cp_info != NULL )
			{
				// Delete our font.
				_DeleteObject( cp_info->hFont_fingerprint_prompt );

				GlobalFree( cp_info->cp_message );
				GlobalFree( cp_info );
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
