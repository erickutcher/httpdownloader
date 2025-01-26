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

#include "folder_browser.h"
#include "lite_shell32.h"
#include "lite_ole32.h"

#include "string_tables.h"
#include "utilities.h"
#include "cmessagebox.h"

bool use_file_open_dialog = true;	// Assume OLE32 was initialized.
bool use_fallback_file_open_dialog = false;

void _BrowseForFolder( HWND hWnd, wchar_t *title, wchar_t **folder )
{
	*folder = NULL;

	#ifndef OLE32_USE_STATIC_LIB
	if ( ole32_state == OLE32_STATE_SHUTDOWN )
	{
		use_file_open_dialog = InitializeOle32();
	}
	#endif

	if ( use_file_open_dialog )
	{
		if ( !use_fallback_file_open_dialog )
		{
			_IFileOpenDialog *dialog = NULL;

			_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

			_CoCreateInstance( _CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, _IID_IFileOpenDialog, ( void ** )&dialog );

			if ( dialog != NULL )
			{
				_IShellItem *isi_result;
				DWORD options;

				dialog->lpVtbl->GetOptions( dialog, &options );
				dialog->lpVtbl->SetOptions( dialog, options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST );
				dialog->lpVtbl->SetTitle( dialog, title );

				if ( SUCCEEDED( dialog->lpVtbl->Show( dialog, hWnd ) ) )
				{
					wchar_t *t_folder = NULL;

					dialog->lpVtbl->GetResult( dialog, ( IShellItem ** )&isi_result );

					isi_result->lpVtbl->GetDisplayName( isi_result, SIGDN_FILESYSPATH, &t_folder );

					isi_result->lpVtbl->Release( isi_result );

					int t_folder_length = lstrlenW( t_folder );

					if ( *folder == NULL )
					{
						*folder = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
					}
					_wmemcpy_s( *folder, MAX_PATH, t_folder, t_folder_length );
					*( *folder + t_folder_length ) = 0;	// Sanity.

					_CoTaskMemFree( t_folder );
				}

				dialog->lpVtbl->Release( dialog );
			}
			else
			{
				use_fallback_file_open_dialog = true;
			}

			_CoUninitialize();
		}
	}
	else
	{
		use_fallback_file_open_dialog = true;
	}

	if ( use_fallback_file_open_dialog )
	{
		// Open a browse for folder dialog box.
		BROWSEINFO bi;
		_memzero( &bi, sizeof( BROWSEINFO ) );
		bi.hwndOwner = hWnd;
		bi.lpszTitle = title;
		bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_VALIDATE;

		if ( use_file_open_dialog )
		{
			// OleInitialize calls CoInitializeEx
			_OleInitialize( NULL );
		}

		LPITEMIDLIST lpiidl = _SHBrowseForFolderW( &bi );
		if ( lpiidl )
		{
			if ( *folder == NULL )
			{
				*folder = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
			}

			// Get the directory path from the id list.
			if ( _SHGetPathFromIDListW( lpiidl, ( LPTSTR )*folder ) == FALSE )
			{
				GlobalFree( *folder );
				*folder = NULL;
			}

			if ( use_file_open_dialog )
			{
				_CoTaskMemFree( lpiidl );	// We're still able to get the folder if OLE32 was not loaded, but if it was, we need to free the item ID list memory.
			}
			else	// Warn of leak if we can't free.
			{
				CMessageBoxW( hWnd, ST_V_Item_ID_List_was_not_freed, PROGRAM_CAPTION, CMB_OK );
			}
		}

		if ( use_file_open_dialog )
		{
			_OleUninitialize();
		}
	}
}
