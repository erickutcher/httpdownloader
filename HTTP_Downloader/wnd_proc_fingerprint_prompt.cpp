/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
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
#include "lite_winmm.h"
#include "string_tables.h"
#include "sftp.h"
#include "cmessagebox.h"

#define BTN_FP_YES				1000
#define BTN_FP_NO				1001

LRESULT CALLBACK FingerprintPromptWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			CPROMPT_INFO *cp_info = ( CPROMPT_INFO * )( ( CREATESTRUCT * )lParam )->lpCreateParams;
			if ( cp_info != NULL && cp_info->data != NULL )
			{
				KEY_PROMPT_INFO *kpi = ( KEY_PROMPT_INFO * )cp_info->data;

				RECT rc, rc_icon;
				_GetClientRect( hWnd, &rc );

				HWND hWnd_static_fp_icon = _CreateWindowW( WC_STATIC, NULL, SS_ICON | WS_CHILD | WS_VISIBLE, 10, 10, 0, 0, hWnd, NULL, NULL, NULL );

				bool play = true;
				#ifndef WINMM_USE_STATIC_LIB
					if ( winmm_state == WINMM_STATE_SHUTDOWN )
					{
						play = InitializeWinMM();
					}
				#endif

				if ( ( cp_info->type & 0xF0 ) == CMB_ICONQUESTION )
				{
					_SendMessageW( hWnd_static_fp_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_QUESTION ), 0 );

					if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMQUESTION, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
				}
				else if ( ( cp_info->type & 0xF0 ) == CMB_ICONHAND )
				{
					_SendMessageW( hWnd_static_fp_icon, STM_SETICON, ( WPARAM )_LoadIconW( NULL, IDI_HAND ), 0 );

					if ( play ) { _PlaySoundW( ( LPCTSTR )SND_ALIAS_SYSTEMHAND, NULL, SND_ASYNC | SND_ALIAS_ID | SND_NODEFAULT ); }
				}

				_GetClientRect( hWnd_static_fp_icon, &rc_icon );

				HWND hWnd_static_fp_msg = _CreateWindowW( WC_STATIC, cp_info->cp_message, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 10, rc.right - ( rc_icon.right * 2 ), 35, hWnd, NULL, NULL, NULL );

				HWND hWnd_static_fp_host = _CreateWindowW( WC_STATIC, ST_V_Host_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 53, 110, 15, hWnd, NULL, NULL, NULL );
				cp_info->hWnd_host = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_host, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 50, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

				HWND hWnd_static_fp_key_algorithm = _CreateWindowW( WC_STATIC, ST_V_Host_key_algorithm_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 81, 110, 15, hWnd, NULL, NULL, NULL );
				cp_info->hWnd_key_algorithm = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_algorithm, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 78, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

				HWND hWnd_static_fp_key_size = _CreateWindowW( WC_STATIC, ST_V_Key_size_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 109, 110, 15, hWnd, NULL, NULL, NULL );
				cp_info->hWnd_key_size = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_size, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 106, rc.right - 155 - rc_icon.right, 23, hWnd, NULL, NULL, NULL );

				HWND hWnd_static_fp_key_fingerprints = _CreateWindowW( WC_STATIC, ST_V_Fingerprints_, WS_CHILD | WS_VISIBLE, rc_icon.right + 20, 137, 110, 15, hWnd, NULL, NULL, NULL );
				cp_info->hWnd_key_fingerprints = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, kpi->w_key_fingerprints, ES_AUTOHSCROLL | ES_READONLY | ES_MULTILINE | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc_icon.right + 145, 134, rc.right - 155 - rc_icon.right, 46, hWnd, NULL, NULL, NULL );

				cp_info->hWnd_checkbox = _CreateWindowW( WC_BUTTON, ST_V_Add_host_and_key_information_to_cache, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, rc.bottom - 32, rc.right - 190, 20, hWnd, NULL, NULL, NULL );

				HWND hWnd_fp_yes = _CreateWindowW( WC_BUTTON, ST_V_Yes, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_FP_YES, NULL, NULL );
				HWND hWnd_fp_no = _CreateWindowW( WC_BUTTON, ST_V_No, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_FP_NO, NULL, NULL );

				_SendMessageW( hWnd_static_fp_msg, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_static_fp_host, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( cp_info->hWnd_host, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_static_fp_key_algorithm, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( cp_info->hWnd_key_algorithm, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_static_fp_key_size, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( cp_info->hWnd_key_size, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_static_fp_key_fingerprints, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( cp_info->hWnd_key_fingerprints, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( cp_info->hWnd_checkbox, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_fp_yes, WM_SETFONT, ( WPARAM )g_hFont, 0 );
				_SendMessageW( hWnd_fp_no, WM_SETFONT, ( WPARAM )g_hFont, 0 );

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
