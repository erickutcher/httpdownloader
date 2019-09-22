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

#include "globals.h"
#include "lite_gdi32.h"
#include "list_operations.h"
#include "string_tables.h"

#include "login_manager_utilities.h"

#define BTN_ADD_LOGIN				1000
#define BTN_REMOVE_LOGIN			1001
#define CHK_SHOW_PASSWORDS			1002
#define BTN_CLOSE_LOGIN_MANAGER_WND	1003

#define EDIT_SITE_LOGIN				1004
#define EDIT_USERNAME_LOGIN			1005
#define EDIT_PASSWORD_LOGIN			1006

#define MENU_LM_REMOVE_SEL			2000
#define MENU_LM_SELECT_ALL			2001

HWND g_hWnd_login_manager = NULL;

HWND g_hWnd_login_list = NULL;

HWND g_hWnd_static_lm_site = NULL;
HWND g_hWnd_edit_lm_site = NULL;

HWND g_hWnd_static_lm_username = NULL;
HWND g_hWnd_edit_lm_username = NULL;
HWND g_hWnd_static_lm_password = NULL;
HWND g_hWnd_edit_lm_password = NULL;

HWND g_hWnd_add_login = NULL;
HWND g_hWnd_remove_login = NULL;
HWND g_hWnd_chk_show_passwords = NULL;
HWND g_hWnd_close_lm_wnd = NULL;

HMENU g_hMenuSub_login_manager = NULL;

bool skip_login_list_draw = false;

wchar_t g_default_pw_char = 0x25CF;	// Bullet
bool g_show_passwords = false;

WNDPROC LoginListProc = NULL;

LRESULT CALLBACK LoginListSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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
						largest_width = 26;	// 5 + 16 + 5.

						wchar_t tbuf[ 11 ];
						wchar_t *buf = NULL;

						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );

						int index = ( int )_SendMessageW( hWnd, LVM_GETTOPINDEX, 0, 0 );
						int index_end = ( int )_SendMessageW( hWnd, LVM_GETCOUNTPERPAGE, 0, 0 ) + index;

						RECT rc;
						HDC hDC = _GetDC( hWnd );
						HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
						_DeleteObject( ohf );

						for ( ; index <= index_end; ++index )
						{
							lvi.iItem = index;
							lvi.mask = LVIF_PARAM;
							if ( _SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi ) == TRUE )
							{
								LOGIN_INFO *li = ( LOGIN_INFO * )lvi.lParam;
								if ( li != NULL )
								{
									// Save the appropriate text in our buffer for the current column.
									switch ( nmh->iItem )
									{
										case 0:
										{
											buf = tbuf;	// Reset the buffer pointer.

											__snwprintf( buf, 11, L"%lu", index + 1 );
										}
										break;

										case 1: { buf = li->w_host; } break;
										case 2: { buf = li->w_username; } break;
										case 3: { buf = ( g_show_passwords ? li->w_password : L"********" ); } break;
									}

									if ( buf == NULL )
									{
										tbuf[ 0 ] = L'\0';
										buf = tbuf;
									}

									rc.bottom = rc.left = rc.right = rc.top = 0;

									_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

									int width = ( rc.right - rc.left ) + 10;	// 5 + 5 padding.
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
	}

	return _CallWindowProcW( LoginListProc, hWnd, msg, wParam, lParam );
}

// Sort function for columns.
int CALLBACK LMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	SORT_INFO *si = ( SORT_INFO * )lParamSort;

	if ( si->hWnd == g_hWnd_login_list )
	{
		LOGIN_INFO *li1 = ( LOGIN_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		LOGIN_INFO *li2 = ( LOGIN_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		switch ( si->column )
		{
			case 1: { return lstrcmpW( li1->w_host, li2->w_host ); } break;
			case 2: { return lstrcmpW( li1->w_username, li2->w_username ); } break;
			case 3:
			{
				if ( g_show_passwords )
				{
					return lstrcmpW( li1->w_password, li2->w_password );
				}
			}
			break;

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

LRESULT CALLBACK LoginManagerWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_login_list = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_login_list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );

			g_hWnd_static_lm_site = _CreateWindowW( WC_STATIC, ST_V_Site_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_lm_site = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SITE_LOGIN, NULL, NULL );

			g_hWnd_static_lm_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_lm_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_USERNAME_LOGIN, NULL, NULL );

			g_hWnd_static_lm_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_lm_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_PASSWORD_LOGIN, NULL, NULL );

			g_hWnd_add_login = _CreateWindowW( WC_BUTTON, ST_V_Add, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_LOGIN, NULL, NULL );
			g_hWnd_remove_login = _CreateWindowW( WC_BUTTON, ST_V_Remove_login, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_REMOVE_LOGIN, NULL, NULL );
			g_hWnd_chk_show_passwords = _CreateWindowW( WC_BUTTON, ST_V_Show_passwords, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SHOW_PASSWORDS, NULL, NULL );
			g_hWnd_close_lm_wnd = _CreateWindowW( WC_BUTTON, ST_V_Close, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CLOSE_LOGIN_MANAGER_WND, NULL, NULL );

			_SendMessageW( g_hWnd_edit_lm_site, EM_LIMITTEXT, MAX_DOMAIN_LENGTH + 8, 0 );	// Include "https://"

			g_default_pw_char = ( wchar_t )_SendMessageW( g_hWnd_edit_lm_password, EM_GETPASSWORDCHAR, 0, 0 );

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = ST_V_NUM;
			lvc.cx = 35;
			_SendMessageW( g_hWnd_login_list, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Site;
			lvc.cx = 240;
			_SendMessageW( g_hWnd_login_list, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Username;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_login_list, LVM_INSERTCOLUMN, 2, ( LPARAM )&lvc );

			lvc.pszText = ST_V_Password;
			lvc.cx = 140;
			_SendMessageW( g_hWnd_login_list, LVM_INSERTCOLUMN, 3, ( LPARAM )&lvc );



			g_hMenuSub_login_manager = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Remove;
			mii.cch = 6;
			mii.wID = MENU_LM_REMOVE_SEL;
			mii.fState = MFS_ENABLED;
			_InsertMenuItemW( g_hMenuSub_login_manager, 0, TRUE, &mii );

			mii.fType = MFT_SEPARATOR;
			_InsertMenuItemW( g_hMenuSub_login_manager, 1, TRUE, &mii );

			mii.fType = MFT_STRING;
			mii.dwTypeData = ST_V_Select_All;
			mii.cch = 10;
			mii.wID = MENU_LM_SELECT_ALL;
			_InsertMenuItemW( g_hMenuSub_login_manager, 2, TRUE, &mii );



			_SendMessageW( g_hWnd_login_list, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_lm_site, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_lm_site, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_lm_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_lm_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_lm_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_edit_lm_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_add_login, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_remove_login, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_passwords, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_close_lm_wnd, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			LoginListProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_login_list, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_login_list, GWLP_WNDPROC, ( LONG_PTR )LoginListSubProc );

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, load_login_list, ( void * )NULL, 0, NULL ) );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_ADD_LOGIN:
				{
					LOGIN_UPDATE_INFO *lui = ( LOGIN_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( LOGIN_UPDATE_INFO ) );
					lui->update_type = 0;	// Add

					LOGIN_INFO *li = ( LOGIN_INFO * )GlobalAlloc( GPTR, sizeof( LOGIN_INFO ) );

					unsigned int text_length = ( unsigned int )_SendMessageW( g_hWnd_edit_lm_site, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( text_length >= 10 )
					{
						li->w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length + 1 );	// Include the NULL terminator.
						_SendMessageW( g_hWnd_edit_lm_site, WM_GETTEXT, text_length + 1, ( LPARAM )li->w_host );

						// Make sure the domain doesn't exceed 253 characters for HTTP and HTTPS.
						// Check to see if it's not an HTTPS site. If it isn't then an extra character could have been added.
						if ( text_length >= ( MAX_DOMAIN_LENGTH + 8 ) && li->w_host[ 4 ] == ':' )
						{
							li->w_host[ text_length - 1 ] = 0;	// Sanity.
						}

						text_length = ( unsigned int )_SendMessageW( g_hWnd_edit_lm_username, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
						li->w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length );
						_SendMessageW( g_hWnd_edit_lm_username, WM_GETTEXT, text_length, ( LPARAM )li->w_username );

						text_length = ( unsigned int )_SendMessageW( g_hWnd_edit_lm_password, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
						li->w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length );
						_SendMessageW( g_hWnd_edit_lm_password, WM_GETTEXT, text_length, ( LPARAM )li->w_password );

						lui->li = li;

						// lui is freed in handle_login_list.
						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_login_list, ( void * )lui, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( lui->li->w_host );
							GlobalFree( lui->li->w_username );
							GlobalFree( lui->li->w_password );
							GlobalFree( lui );
						}
					}
					else
					{
						_MessageBoxW( hWnd, ST_V_The_specified_site_is_invalid, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
					}
				}
				break;

				case BTN_REMOVE_LOGIN:
				case MENU_LM_REMOVE_SEL:
				{
					if ( _MessageBoxW( hWnd, ST_V_PROMPT_remove_selected_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
					{
						LOGIN_UPDATE_INFO *lui = ( LOGIN_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( LOGIN_UPDATE_INFO ) );
						lui->update_type = 1;	// Remove
						lui->li = NULL;

						HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_login_list, ( void * )lui, 0, NULL );
						if ( thread != NULL )
						{
							CloseHandle( thread );
						}
						else
						{
							GlobalFree( lui );
						}
					}
				}
				break;

				case BTN_CLOSE_LOGIN_MANAGER_WND:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_SITE_LOGIN:
				case EDIT_USERNAME_LOGIN:
				case EDIT_PASSWORD_LOGIN:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						if ( ( _SendMessageW( g_hWnd_edit_lm_site, WM_GETTEXTLENGTH, 0, 0 ) > 0 ) && 
						   ( ( _SendMessageW( g_hWnd_edit_lm_username, WM_GETTEXTLENGTH, 0, 0 ) > 0 ) ||
							 ( _SendMessageW( g_hWnd_edit_lm_password, WM_GETTEXTLENGTH, 0, 0 ) > 0 ) ) )
						{
							_EnableWindow( g_hWnd_add_login, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_add_login, FALSE );
						}
					}
				}
				break;

				case MENU_LM_SELECT_ALL:
				{
					// Set the state of all items to selected.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_STATE;
					lvi.state = LVIS_SELECTED;
					lvi.stateMask = LVIS_SELECTED;
					_SendMessageW( g_hWnd_login_list, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );
				}
				break;

				case CHK_SHOW_PASSWORDS:
				{
					g_show_passwords = ( _SendMessageW( g_hWnd_chk_show_passwords, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					
					_SendMessageW( g_hWnd_edit_lm_password, EM_SETPASSWORDCHAR, ( g_show_passwords ? 0 : g_default_pw_char ), 0 );
					_InvalidateRect( g_hWnd_edit_lm_password, NULL, FALSE );

					_InvalidateRect( g_hWnd_login_list, NULL, FALSE );
				}
				break;
			}

			return 0;
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

					_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )LMCompareFunc );
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_login_list )
					{
						POINT p;
						_GetCursorPos( &p );

						int item_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 );
						int sel_count = ( int )_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 );
						
						_EnableMenuItem( g_hMenuSub_login_manager, MENU_LM_REMOVE_SEL, ( sel_count > 0 ? MF_ENABLED : MF_GRAYED ) );
						_EnableMenuItem( g_hMenuSub_login_manager, MENU_LM_SELECT_ALL, ( sel_count == item_count ? MF_GRAYED : MF_ENABLED ) );

						_TrackPopupMenu( g_hMenuSub_login_manager, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

				case NM_DBLCLK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_login_list )
					{
						LVITEM lvi;
						_memzero( &lvi, sizeof( LVITEM ) );
						lvi.iItem = nmitem->iItem;
						lvi.mask = LVIF_PARAM;
						_SendMessageW( nmitem->hdr.hwndFrom, LVM_GETITEM, 0, ( LPARAM )&lvi );

						LOGIN_INFO *li = ( LOGIN_INFO * )lvi.lParam;
						if ( li != NULL )
						{
							_SendMessageW( g_hWnd_edit_lm_site, WM_SETTEXT, 0, ( LPARAM )li->w_host );
							_SendMessageW( g_hWnd_edit_lm_username, WM_SETTEXT, 0, ( LPARAM )li->w_username );
							_SendMessageW( g_hWnd_edit_lm_password, WM_SETTEXT, 0, ( LPARAM )li->w_password );
						}
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
									_SendMessageW( hWnd, WM_COMMAND, MENU_LM_SELECT_ALL, 0 );
								}
							}
							break;
						}
					}
					else if ( ( ( LPNMLVKEYDOWN )lParam )->wVKey == VK_DELETE )	// Remove items if Delete is down and there are selected items in the list.
					{
						if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_LM_REMOVE_SEL, 0 );
						}
					}
				}
				break;

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
					{
						_EnableWindow( g_hWnd_remove_login, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_remove_login, FALSE );
					}
				}
				break;
			}
			return FALSE;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				LOGIN_INFO *li = ( LOGIN_INFO * )dis->itemData;

				// If an item is being deleted, then don't draw it.
				if ( skip_login_list_draw )
				{
					return TRUE;
				}

				// Alternate item color's background.
				if ( dis->itemID & 1 )	// Even rows will have a light grey background.
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				wchar_t tbuf[ 11 ];
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < 4; ++i )
				{
					// Save the appropriate text in our buffer for the current column.
					switch ( i )
					{
						case 0:
						{
							buf = tbuf;	// Reset the buffer pointer.

							__snwprintf( buf, 11, L"%lu", dis->itemID + 1 );
						}
						break;

						case 1: { buf = li->w_host; } break;
						case 2: { buf = li->w_username; } break;
						case 3: { buf = ( g_show_passwords ? li->w_password : L"********" ); }
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
					last_rc.left = 5 + last_left;
					last_rc.right = lvc.cx + last_left - 5;

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
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, g_hFont );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					// Draw selected text
					if ( selected )
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// White text.
						_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOW ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
					}
					else	// Draw normal text.
					{
						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						// Black text.
						_SetTextColor( hdcMem, _GetSysColor( COLOR_WINDOWTEXT ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}
			}
			return TRUE;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = MIN_WIDTH;
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = MIN_HEIGHT;
			
			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 11 );
			_DeferWindowPos( hdwp, g_hWnd_login_list, HWND_TOP, 10, 10, rc.right - 20, rc.bottom - 112, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_lm_site, HWND_TOP, 10, rc.bottom - 92, rc.right - 240, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_lm_site, HWND_TOP, 10, rc.bottom - 77, rc.right - 240, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_lm_username, HWND_TOP, rc.right - 220, rc.bottom - 92, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_lm_username, HWND_TOP, rc.right - 220, rc.bottom - 77, 100, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_lm_password, HWND_TOP, rc.right - 110, rc.bottom - 92, 100, 15, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_lm_password, HWND_TOP, rc.right - 110, rc.bottom - 77, 100, 23, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_add_login, HWND_TOP, 10, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_remove_login, HWND_TOP, 95, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_show_passwords, HWND_TOP, 180, rc.bottom - 32, 130, 23, SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_close_lm_wnd, HWND_TOP, rc.right - 90, rc.bottom - 32, 80, 23, SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType = ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = g_default_row_height;
			}
			return TRUE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				_MessageBoxW( hWnd, ST_V_The_specified_site_already_exists, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
			}
			else if ( wParam == 2 )
			{
				_MessageBoxW( hWnd, ST_V_A_protocol_must_be_supplied, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
			}
			else if ( wParam == 3 )
			{
				_EnableWindow( g_hWnd_remove_login, FALSE );
			}
			else
			{
				_SendMessageW( g_hWnd_edit_lm_site, WM_SETTEXT, 0, 0 );
				_SendMessageW( g_hWnd_edit_lm_username, WM_SETTEXT, 0, 0 );
				_SendMessageW( g_hWnd_edit_lm_password, WM_SETTEXT, 0, 0 );

				_SetFocus( g_hWnd_edit_lm_site );
			}
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

			_SetFocus( g_hWnd_login_list );

            return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			g_show_passwords = false;

			_DestroyMenu( g_hMenuSub_login_manager );

			g_hWnd_login_list = NULL;
			g_hWnd_login_manager = NULL;

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
