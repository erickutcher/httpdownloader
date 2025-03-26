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

#include "options.h"
#include "sftp.h"
#include "lite_gdi32.h"
#include "cmessagebox.h"

#define EDIT_SFH_HOST			1000
#define EDIT_SFH_FINGERPRINT	1001
#define CB_SFH_KEY_ALGORITHM	1002
#define BTN_SFH_NEW_HOST		1003
#define BTN_SFH_SAVE_HOST		1004
#define BTN_SFH_REMOVE_HOST		1005

#define MENU_SFH_REMOVE_SEL		2000
#define MENU_SFH_SELECT_ALL		2001

// SFTP Tab
HWND g_hWnd_static_sftp_fps_host = NULL;
HWND g_hWnd_static_sftp_fps_key_algorithm = NULL;
HWND g_hWnd_static_sftp_fps_fingerprint = NULL;

HWND g_hWnd_sftp_fps_host_list = NULL;

HWND g_hWnd_edit_sftp_fps_host = NULL;
HWND g_hWnd_cb_sftp_fps_key_algorithm = NULL;
HWND g_hWnd_edit_sftp_fps_fingerprint = NULL;
HWND g_hWnd_new_fps_host = NULL;
HWND g_hWnd_save_fps_host = NULL;
HWND g_hWnd_remove_fps_host = NULL;

bool skip_sftp_fps_host_list_draw = false;

SFTP_FPS_HOST_INFO *g_selected_fps_host_info = NULL;
int g_selected_fps_host_index = -1;

HMENU g_hMenuSub_sftp_fps = NULL;

WNDPROC SFHListProc = NULL;

int CALLBACK SFHCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	SORT_INFO *si = ( SORT_INFO * )lParamSort;

	if ( si->hWnd == g_hWnd_sftp_fps_host_list )
	{
		SFTP_FPS_HOST_INFO *sfhi1 = ( SFTP_FPS_HOST_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		SFTP_FPS_HOST_INFO *sfhi2 = ( SFTP_FPS_HOST_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		switch ( si->column )
		{
			case 1:
			{
				int ret = lstrcmpA( sfhi1->host, sfhi2->host );

				if ( ret == 0 )
				{
					if ( sfhi1->port > sfhi2->port )
					{
						return 1;
					}
					else if ( sfhi1->port < sfhi2->port )
					{
						return -1;
					}
				}

				return ret;
			}
			break;

			case 2: { return lstrcmpW( sfhi1->w_key_algorithm, sfhi2->w_key_algorithm ); } break;

			case 3: { return lstrcmpW( sfhi1->w_key_fingerprint, sfhi2->w_key_fingerprint ); } break;

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

void SelectSFTPFpsHostItem( int index )
{
	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.iItem = index;
	lvi.mask = LVIF_PARAM | LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

	SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )lvi.lParam;
	if ( sfhi != NULL && ( lvi.state & LVIS_SELECTED ) )
	{
		if ( sfhi == g_selected_fps_host_info )
		{
			return;
		}

		g_selected_fps_host_info = sfhi;
		g_selected_fps_host_index = index;

		_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETTEXT, 0, ( LPARAM )sfhi->w_host );
		_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETTEXT, 0, ( LPARAM )sfhi->w_key_algorithm );
		_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETTEXT, 0, ( LPARAM )sfhi->w_key_fingerprint );
	}
	else
	{
		g_selected_fps_host_info = NULL;
		g_selected_fps_host_index = -1;

		_EnableWindow( g_hWnd_remove_fps_host, FALSE );

		_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETTEXT, 0, NULL );
		_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETTEXT, 0, NULL );
	}
}

LRESULT CALLBACK SFHListSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_DIVIDERDBLCLICK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					int largest_width;

					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						largest_width = LVSCW_AUTOSIZE_USEHEADER;
					}
					else
					{
						// Need to scale each number for rounding purposes.
						largest_width = _SCALE_O_( 5 ) + _SCALE_O_( 16 ) + _SCALE_O_( 5 );

						wchar_t tbuf[ 128 ];
						wchar_t *buf = NULL;

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );

						int index = ( int )_SendMessageW( hWnd, LVM_GETTOPINDEX, 0, 0 );
						int index_end = ( int )_SendMessageW( hWnd, LVM_GETCOUNTPERPAGE, 0, 0 ) + index;

						RECT rc;
						HDC hDC = _GetDC( hWnd );
						HFONT ohf = ( HFONT )_SelectObject( hDC, hFont_options );
						_DeleteObject( ohf );

						for ( ; index <= index_end; ++index )
						{
							lvi.iItem = index;
							lvi.mask = LVIF_PARAM;
							if ( _SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi ) == TRUE )
							{
								SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )lvi.lParam;
								if ( sfhi != NULL )
								{
									switch ( nmh->iItem )
									{
										case 0:	// Num
										{
											buf = tbuf;	// Reset the buffer pointer.

											__snwprintf( buf, 128, L"%lu", nmh->iItem + 1 );
										}
										break;

										case 1:	// Host
										{
											buf = sfhi->w_host;
										}
										break;

										case 2:	// Key Algorithm
										{
											buf = sfhi->w_key_algorithm;
										}
										break;

										case 3:	// Key
										{
											buf = sfhi->w_key_fingerprint;
										}
										break;
									}

									if ( buf == NULL )
									{
										tbuf[ 0 ] = L'\0';
										buf = tbuf;
									}

									rc.bottom = rc.left = rc.right = rc.top = 0;

									_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

									// Need to scale each number for rounding purposes.
									int width = ( rc.right - rc.left ) + _SCALE_O_( 5 ) + _SCALE_O_( 5 );	// 5 + 5 padding.
									if ( width > largest_width )
									{
										largest_width = width;
									}
								}
							}
							else
							{
								break;
							}
						}

						_ReleaseDC( hWnd, hDC );
					}

					_SendMessageW( hWnd, LVM_SETCOLUMNWIDTH, nmh->iItem, largest_width );

					return TRUE;
				}
				break;
			}
		}
		break;

		case WM_KEYDOWN:
		{
			switch ( wParam )
			{
				case VK_APPS:	// Context menu key.
				{
					int item_count = ( int )_SendMessageW( hWnd, LVM_GETITEMCOUNT, 0, 0 );
					int sel_count = ( int )_SendMessageW( hWnd, LVM_GETSELECTEDCOUNT, 0, 0 );

					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM;
					lvi.iItem = ( int )_SendMessageW( hWnd, LVM_GETNEXTITEM, ( WPARAM )-1, LVNI_FOCUSED | LVNI_SELECTED );
					_SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi );

					HWND hWnd_header = ( HWND )_SendMessageW( hWnd, LVM_GETHEADER, 0, 0 );

					RECT rc;
					_GetClientRect( hWnd, &rc );
					WINDOWPOS wp;
					_memzero( &wp, sizeof ( WINDOWPOS ) );
					HDLAYOUT hdl;
					hdl.prc = &rc;
					hdl.pwpos = &wp;
					_SendMessageW( hWnd_header, HDM_LAYOUT, 0, ( LPARAM )&hdl );

					POINT p;
					p.x = ( _SCALE_O_( g_default_row_height ) / 2 );
					p.y = p.x + ( wp.cy - wp.y );

					if ( lvi.iItem != -1 )
					{
						rc.left = LVIR_BOUNDS;
						_SendMessageW( hWnd, LVM_GETITEMRECT, lvi.iItem, ( LPARAM )&rc );

						p.x = rc.left + ( ( rc.bottom - rc.top ) / 2 );
						p.y = rc.top + ( ( rc.bottom - rc.top ) / 2 );
					}

					_ClientToScreen( hWnd, &p );

					_EnableMenuItem( g_hMenuSub_sftp_fps, MENU_SFH_REMOVE_SEL, ( sel_count > 0 ? MF_ENABLED : MF_GRAYED ) );
					_EnableMenuItem( g_hMenuSub_sftp_fps, MENU_SFH_SELECT_ALL, ( sel_count == item_count ? MF_GRAYED : MF_ENABLED ) );

					_TrackPopupMenu( g_hMenuSub_sftp_fps, 0, p.x, p.y, 0, _GetParent( hWnd ), NULL );
				}
			}
		}
		break;
	}

	return _CallWindowProcW( SFHListProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK SFTPFpsTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_sftp_fps_host_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

			g_hWnd_static_sftp_fps_host = _CreateWindowW( WC_STATIC, ST_V_Host_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sftp_fps_host = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SFH_HOST, NULL, NULL );

			g_hWnd_static_sftp_fps_key_algorithm = _CreateWindowW( WC_STATIC, ST_V_Host_key_algorithm_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_cb_sftp_fps_key_algorithm = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWN | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_SFH_KEY_ALGORITHM, NULL, NULL );

			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ecdsa_sha2_nistp256 );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ecdsa_sha2_nistp384 );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ecdsa_sha2_nistp521 );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ssh_dss );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ssh_ed25519 );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, CB_ADDSTRING, 0, ( LPARAM )ST_V_ssh_rsa );

			g_hWnd_static_sftp_fps_fingerprint = _CreateWindowW( WC_STATIC, ST_V_Fingerprint_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_sftp_fps_fingerprint = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SFH_FINGERPRINT, NULL, NULL );

			g_hWnd_new_fps_host = _CreateWindowW( WC_BUTTON, ST_V_New, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SFH_NEW_HOST, NULL, NULL );
			g_hWnd_save_fps_host = _CreateWindowW( WC_BUTTON, ST_V_Save, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SFH_SAVE_HOST, NULL, NULL );
			g_hWnd_remove_fps_host = _CreateWindowW( WC_BUTTON, ST_V_Remove, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SFH_REMOVE_HOST, NULL, NULL );

			_SendMessageW( g_hWnd_edit_sftp_fps_host, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );

			//

			g_hMenuSub_sftp_fps = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Remove;
			mii.cch = ST_L_Remove;
			mii.wID = MENU_SFH_REMOVE_SEL;
			_InsertMenuItemW( g_hMenuSub_sftp_fps, 0, TRUE, &mii );

			mii.fType = MFT_SEPARATOR;
			_InsertMenuItemW( g_hMenuSub_sftp_fps, 1, TRUE, &mii );

			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Select_All;
			mii.cch = ST_L_Select_All;
			mii.wID = MENU_SFH_SELECT_ALL;
			_InsertMenuItemW( g_hMenuSub_sftp_fps, 2, TRUE, &mii );

			//

			_SendMessageW( g_hWnd_sftp_fps_host_list, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_sftp_fps_host, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_sftp_fps_key_algorithm, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_sftp_fps_fingerprint, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_new_fps_host, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_save_fps_host, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_remove_fps_host, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			//

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.pszText = ST_V_NUM;
			lvc.cx = 35;
			_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Host;
			lvc.cx = 150;
			_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Host_Key_Algorithm;
			lvc.cx = 120;
			_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTCOLUMN, 2, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Fingerprint;
			lvc.cx = rc.right - 330;
			_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTCOLUMN, 3, ( LPARAM )&lvc );

			//

			SFHListProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_sftp_fps_host_list, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_sftp_fps_host_list, GWLP_WNDPROC, ( LONG_PTR )SFHListSubProc );

			//

			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, load_sftp_fps_host_list, ( void * )NULL, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HDWP hdwp = _BeginDeferWindowPos( 10 );
			_DeferWindowPos( hdwp, g_hWnd_sftp_fps_host_list, HWND_TOP, 0, 0, rc.right, _SCALE_O_( 330 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sftp_fps_host, HWND_TOP, 0, _SCALE_O_( 337 ), _SCALE_O_( 190 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sftp_fps_host, HWND_TOP, 0, _SCALE_O_( 355 ), _SCALE_O_( 190 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sftp_fps_key_algorithm, HWND_TOP, _SCALE_O_( 195 ), _SCALE_O_( 337 ), _SCALE_O_( 150 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_cb_sftp_fps_key_algorithm, HWND_TOP, _SCALE_O_( 195 ), _SCALE_O_( 355 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_sftp_fps_fingerprint, HWND_TOP, _SCALE_O_( 350 ), _SCALE_O_( 337 ), rc.right - _SCALE_O_( 350 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_sftp_fps_fingerprint, HWND_TOP, _SCALE_O_( 350 ), _SCALE_O_( 355 ), rc.right - _SCALE_O_( 350 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_new_fps_host, HWND_TOP, 0, _SCALE_O_( 388 ), _SCALE_O_( 105 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_save_fps_host, HWND_TOP, _SCALE_O_( 110 ), _SCALE_O_( 388 ), _SCALE_O_( 105 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_remove_fps_host, HWND_TOP, _SCALE_O_( 220 ), _SCALE_O_( 388 ), _SCALE_O_( 105 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_options;
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			for ( int i = 0; i < 4; ++i )
			{
				int column_width = ( int )_SendMessageA( g_hWnd_sftp_fps_host_list, LVM_GETCOLUMNWIDTH, ( WPARAM )i, 0 );
				_SendMessageA( g_hWnd_sftp_fps_host_list, LVM_SETCOLUMNWIDTH, ( WPARAM )i, MAKELPARAM( _SCALE2_( column_width, dpi_options ), 0 ) );
			}

			// Return value is ignored.
			return TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_SFH_NEW_HOST:
				{
					g_selected_fps_host_info = NULL;
					g_selected_fps_host_index = -1;

					_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETTEXT, 0, NULL );
					_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETTEXT, 0, NULL );
					_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETTEXT, 0, NULL );

					// Set the state of all items to deselected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );

					_SetFocus( g_hWnd_edit_sftp_fps_host );
				}
				break;

				case BTN_SFH_SAVE_HOST:
				{
					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_GETTEXTLENGTH, 0, 0 );
					unsigned int edit_ka_length = ( unsigned int )_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_GETTEXTLENGTH, 0, 0 );

					if ( edit_length < 3 )	// a.b
					{
						CMessageBoxW( hWnd, ST_V_The_specified_host_is_invalid, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
					}
					else if ( edit_ka_length < 1 )
					{
						CMessageBoxW( hWnd, ST_V_A_key_algorithm_must_be_supplied, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
					}
					else
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.iItem = g_selected_fps_host_index;
						lvi.mask = LVIF_PARAM | LVIF_STATE;
						lvi.stateMask = LVIS_SELECTED;
						_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

						SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )lvi.lParam;
						if ( sfhi != NULL && ( lvi.state & LVIS_SELECTED ) )
						{
							g_selected_fps_host_info = sfhi;
						}
						else
						{
							g_selected_fps_host_info = NULL;
							g_selected_fps_host_index = -1;
						}

						SFTP_FPS_HOST_UPDATE_INFO *sfhui = ( SFTP_FPS_HOST_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SFTP_FPS_HOST_UPDATE_INFO ) );
						if ( sfhui != NULL )
						{
							sfhui->update_type = 0;	// Add
							sfhui->old_sfhi = g_selected_fps_host_info;
							sfhui->index = g_selected_fps_host_index;

							sfhui->sfhi = ( SFTP_FPS_HOST_INFO * )GlobalAlloc( GPTR, sizeof( SFTP_FPS_HOST_INFO ) );
							if ( sfhui->sfhi != NULL )
							{
								sfhui->sfhi->w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );	// Include the NULL terminator.
								_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_GETTEXT, edit_length + 1, ( LPARAM )sfhui->sfhi->w_host );

								sfhui->sfhi->w_key_algorithm = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_ka_length + 1 ) );	// Include the NULL terminator.
								_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_GETTEXT, edit_ka_length + 1, ( LPARAM )sfhui->sfhi->w_key_algorithm );

								edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_GETTEXTLENGTH, 0, 0 );
								sfhui->sfhi->w_key_fingerprint = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );	// Include the NULL terminator.
								_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_GETTEXT, edit_length + 1, ( LPARAM )sfhui->sfhi->w_key_fingerprint );

								//

								// sfhui is freed in handle_sftp_fps_host_list.
								HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_sftp_fps_host_list, ( void * )sfhui, 0, NULL );
								if ( thread != NULL )
								{
									CloseHandle( thread );
								}
								else
								{
									FreeSFTPFpsHostInfo( &sfhui->sfhi );
									GlobalFree( sfhui );
								}
							}
							else
							{
								GlobalFree( sfhui );
							}
						}
					}
				}
				break;

				case BTN_SFH_REMOVE_HOST:
				case MENU_SFH_REMOVE_SEL:
				{
					if ( CMessageBoxW( hWnd, ST_V_PROMPT_remove_selected_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
					{
						SFTP_FPS_HOST_UPDATE_INFO *sfhui = ( SFTP_FPS_HOST_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SFTP_FPS_HOST_UPDATE_INFO ) );
						if ( sfhui != NULL )
						{
							sfhui->update_type = 1;	// Remove
							sfhui->sfhi = NULL;

							// sfhui is freed in handle_sftp_fps_host_list.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_sftp_fps_host_list, ( void * )sfhui, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								GlobalFree( sfhui );
							}
						}
					}
				}
				break;

				case MENU_SFH_SELECT_ALL:
				{
					// Set the state of all items to selected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.state = LVIS_SELECTED;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dis->itemData;

				// If an item is being deleted, then don't draw it.
				if ( skip_sftp_fps_host_list_draw )
				{
					return TRUE;
				}

#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					// Alternate item color's background.
					HBRUSH color;
					if ( dis->itemID & 1 )	// Even rows will have a dark grey background.
					{
						color = _CreateSolidBrush( dm_color_edit_background );
					}
					else
					{
						color = _CreateSolidBrush( ( COLORREF )RGB( 0x00, 0x00, 0x00 ) );
					}
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}
				else
#endif
				{
					// Alternate item color's background.
					if ( dis->itemID & 1 )	// Even rows will have a light grey background.
					{
						HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
						_FillRect( dis->hDC, &dis->rcItem, color );
						_DeleteObject( color );
					}
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					HBRUSH color;

#ifdef ENABLE_DARK_MODE
					if ( g_use_dark_mode )
					{
						color = _CreateSolidBrush( dm_color_list_highlight );
					}
					else
#endif
					{
						color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					}

					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );

					selected = true;
				}

				wchar_t tbuf[ 128 ];
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int DT_ALIGN = 0;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i <= 3; ++i )
				{
					switch ( i )
					{
						case 0:	// Num
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 128, L"%lu", dis->itemID + 1 );
						}
						break;

						case 1:	// Host
						{
							buf = sfhi->w_host;
						}
						break;

						case 2:	// Key Algorithm
						{
							buf = sfhi->w_key_algorithm;
						}
						break;

						case 3:	// Key
						{
							buf = sfhi->w_key_fingerprint;
						}
						break;
					}

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, i, ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					// This will adjust the text to fit nicely into the rectangle.
					last_rc.left = _SCALE_O_( 5 ) + last_left;
					last_rc.right = lvc.cx + last_left - _SCALE_O_( 5 );

					// Save the last left position of our column.
					last_left += lvc.cx;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					if ( width <= 0 )
					{
						continue;
					}

					int height = last_rc.bottom - last_rc.top;

					// Normal text position.
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = width;
					rc.bottom = height;

					// Create and save a bitmap in memory to paint to.
					HDC hdcMem = _CreateCompatibleDC( dis->hDC );
					HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
					HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
					_DeleteObject( ohbm );
					_DeleteObject( hbm );
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont_options );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					HBRUSH color;

					// Draw selected text
					if ( selected )
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							color = _CreateSolidBrush( dm_color_list_highlight );
						}
						else
#endif
						{
							color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
						}

						// Fill the background.
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// White text.
						_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOW ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							// Fill the background.
							color = _CreateSolidBrush( ( dis->itemID & 1 ? dm_color_edit_background : ( COLORREF )RGB( 0x00, 0x00, 0x00 ) ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// White text.
							_SetTextColor( hdcMem, ( sfhi->temporary ? ( COLORREF )RGB( 0xFF, 0x40, 0x00 ) : dm_color_window_text ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
						}
						else
#endif
						{
							// Fill the background.
							color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// Black text.
							_SetTextColor( hdcMem, ( sfhi->temporary ? ( COLORREF )RGB( 0xFF, 0x40, 0x00 ) : _GetSysColor( COLOR_WINDOWTEXT ) ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
						}
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}

				if ( dis->itemState & ODS_FOCUS )
				{
					DWORD ui_state = ( DWORD )_SendMessageW( hWnd, WM_QUERYUISTATE, 0, 0 );
					if ( !( ui_state & UISF_HIDEFOCUS ) && dis->hwndItem == _GetFocus() )
					{
#ifdef ENABLE_DARK_MODE
						if ( g_use_dark_mode )
						{
							LOGBRUSH lb;
							lb.lbColor = dm_color_focus_rectangle;
							lb.lbStyle = PS_SOLID;
							HPEN hPen = _ExtCreatePen( PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, NULL );
							HPEN old_color = ( HPEN )_SelectObject( dis->hDC, hPen );
							_DeleteObject( old_color );
							HBRUSH old_brush = ( HBRUSH )_SelectObject( dis->hDC, _GetStockObject( NULL_BRUSH ) );
							_DeleteObject( old_brush );
							_Rectangle( dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom - 1 );
							_DeleteObject( hPen );
						}
						else
#endif
						{
							RECT rc;
							rc.left = dis->rcItem.left;
							rc.top = dis->rcItem.top;
							rc.right = dis->rcItem.right;
							rc.bottom = dis->rcItem.bottom - 1;
							_DrawFocusRect( dis->hDC, &rc );
						}
					}
				}
			}

			return TRUE;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					LVCOLUMN lvc;
					_memzero( &lvc, sizeof( LVCOLUMN ) );
					lvc.mask = LVCF_FMT | LVCF_ORDER;
					_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

					// The number column doesn't get sorted and its iOrder is always 0, so let's remove any sort arrows that are active.
					if ( lvc.iOrder == 0 )
					{
						// Remove the sort format for all columns.
						for ( unsigned char i = 1; _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc ) == TRUE; ++i )
						{
							// Remove sort up and sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
						}

						break;
					}

					SORT_INFO si;
					si.column = lvc.iOrder;
					si.hWnd = nmlv->hdr.hwndFrom;

					if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
					{
						si.direction = 0;	// Now sort down.

						// Sort down
						lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );
					}
					else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
					{
						si.direction = 1;	// Now sort up.

						// Sort up
						lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
					}
					else	// Column has no sorting set.
					{
						// Remove the sort format for all columns.
						for ( unsigned char i = 0; _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc ) == TRUE; ++i )
						{
							// Remove sort up and sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
						}

						// Read current the format from the clicked column
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						si.direction = 0;	// Start the sort going down.

						// Sort down to start.
						lvc.fmt = lvc.fmt | HDF_SORTDOWN;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
					}

					_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )SFHCompareFunc );
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_sftp_fps_host_list )
					{
						POINT p;
						_GetCursorPos( &p );

						int item_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 );
						int sel_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 );

						_EnableMenuItem( g_hMenuSub_sftp_fps, MENU_SFH_REMOVE_SEL, ( sel_count > 0 ? MF_ENABLED : MF_GRAYED ) );
						_EnableMenuItem( g_hMenuSub_sftp_fps, MENU_SFH_SELECT_ALL, ( sel_count == item_count ? MF_GRAYED : MF_ENABLED ) );

						_TrackPopupMenu( g_hMenuSub_sftp_fps, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( ( nmlv->uNewState != ( LVIS_FOCUSED | LVIS_SELECTED ) ) )
					{
						break;
					}

					if ( nmlv->hdr.hwndFrom == g_hWnd_sftp_fps_host_list )
					{
						SelectSFTPFpsHostItem( nmlv->iItem );
					}
				}
				break;

				case NM_CLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_sftp_fps_host_list )
					{
						if ( nmitem->iItem == -1 )
						{
							if ( ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) == 0 )
							{
								g_selected_fps_host_info = NULL;
								g_selected_fps_host_index = -1;

								_EnableWindow( g_hWnd_remove_fps_host, FALSE );

								_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETTEXT, 0, NULL );
								_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETTEXT, 0, NULL );
								_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETTEXT, 0, NULL );
							}

							break;
						}

						_EnableWindow( g_hWnd_remove_fps_host, TRUE );

						SelectSFTPFpsHostItem( nmitem->iItem );
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 && !in_worker_thread )
					{
						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SFH_SELECT_ALL, 0 );
								}
							}
							break;
						}
					}
					else if ( ( ( LPNMLVKEYDOWN )lParam )->wVKey == VK_DELETE )	// Remove items if Delete is down and there are selected items in the list.
					{
						if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_SFH_REMOVE_SEL, 0 );
						}
					}
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType == ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = _SCALE_O_( g_default_row_height );
			}
			return TRUE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				CMessageBoxW( hWnd, ST_V_The_specified_host_already_exists, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
			}
			else if ( wParam == 2 )
			{
				g_selected_fps_host_info = NULL;
				g_selected_fps_host_index = -1;

				_EnableWindow( g_hWnd_remove_fps_host, FALSE );

				_SendMessageW( g_hWnd_edit_sftp_fps_host, WM_SETTEXT, 0, NULL );
				_SendMessageW( g_hWnd_cb_sftp_fps_key_algorithm, WM_SETTEXT, 0, NULL );
				_SendMessageW( g_hWnd_edit_sftp_fps_fingerprint, WM_SETTEXT, 0, NULL );
			}
			else if ( wParam == 3 )
			{
				SFTP_FPS_HOST_UPDATE_INFO *sfhui = ( SFTP_FPS_HOST_UPDATE_INFO * )lParam;
				if ( sfhui->sfhi != NULL )
				{
					g_selected_fps_host_info = sfhui->sfhi;
					g_selected_fps_host_index = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 ) - 1;

					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					lvi.state = 0;
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETITEMSTATE, ( WPARAM )-1, ( LPARAM )&lvi );	// Clear all states.

					lvi.state = LVIS_SELECTED;
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETITEMSTATE, g_selected_fps_host_index, ( LPARAM )&lvi );

					_EnableWindow( g_hWnd_remove_fps_host, TRUE );
				}
			}

			_SetFocus( g_hWnd_edit_sftp_fps_host );

			return TRUE;
		}
		break;

		case WM_DESTROY:
		{
			g_selected_fps_host_info = NULL;
			g_selected_fps_host_index = -1;

			_DestroyMenu( g_hMenuSub_sftp_fps );

			g_hWnd_sftp_fps_host_list = NULL;	// We might need to add items from the FingerprintPrompt thread.

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
