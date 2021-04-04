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

#ifndef _FOLDER_BROWSER_H
#define _FOLDER_BROWSER_H

#include <shobjidl.h>

//const IID _IID_IUnknown =			{ 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
//const IID _IID_IFileDialog =		{ 0x42F85136, 0xDB7E, 0x439C, { 0x85, 0xF1, 0xE4, 0x07, 0x5D, 0x13, 0x5F, 0xC8 } };
const IID _IID_IFileOpenDialog =	{ 0xD57C7288, 0xD4AD, 0x4768, { 0xBE, 0x02, 0x9D, 0x96, 0x95, 0x32, 0xD9, 0x60 } };

const CLSID _CLSID_FileOpenDialog = { 0xDC1C5A9C, 0xE88A, 0x4DDE, { 0xA5, 0xA1, 0x60, 0xF8, 0x2A, 0x20, 0xAE, 0xF7 } };

typedef DWORD FILEOPENDIALOGOPTIONS;

/*
struct _IFileDialog;

typedef struct IFileDialogVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( _IFileDialog *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( _IFileDialog *This );
	ULONG ( STDMETHODCALLTYPE *Release )( _IFileDialog *This );
	HRESULT ( STDMETHODCALLTYPE *Show )( _IFileDialog *This, HWND hwndOwner );
	HRESULT ( STDMETHODCALLTYPE *SetFileTypes )( _IFileDialog *This, UINT cFileTypes, const COMDLG_FILTERSPEC *rgFilterSpec );
	HRESULT ( STDMETHODCALLTYPE *SetFileTypeIndex )( _IFileDialog *This, UINT iFileType );
	HRESULT ( STDMETHODCALLTYPE *GetFileTypeIndex )( _IFileDialog *This, UINT *piFileType );
	HRESULT ( STDMETHODCALLTYPE *Advise )( _IFileDialog *This, IFileDialogEvents *pfde, DWORD *pdwCookie );
	HRESULT ( STDMETHODCALLTYPE *Unadvise )( _IFileDialog *This, DWORD dwCookie );
	HRESULT ( STDMETHODCALLTYPE *SetOptions )( _IFileDialog *This, FILEOPENDIALOGOPTIONS fos );
	HRESULT ( STDMETHODCALLTYPE *GetOptions )( _IFileDialog *This, FILEOPENDIALOGOPTIONS *pfos );
	HRESULT ( STDMETHODCALLTYPE *SetDefaultFolder )( _IFileDialog *This, IShellItem *psi );
	HRESULT ( STDMETHODCALLTYPE *SetFolder )( _IFileDialog *This, IShellItem *psi );
	HRESULT ( STDMETHODCALLTYPE *GetFolder )( _IFileDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *GetCurrentSelection )( _IFileDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *SetFileName )( _IFileDialog *This, LPCWSTR pszName );
	HRESULT ( STDMETHODCALLTYPE *GetFileName )( _IFileDialog *This, LPWSTR *pszName );
	HRESULT ( STDMETHODCALLTYPE *SetTitle )( _IFileDialog *This, LPCWSTR pszTitle );
	HRESULT ( STDMETHODCALLTYPE *SetOkButtonLabel )( _IFileDialog *This, LPCWSTR pszText );
	HRESULT ( STDMETHODCALLTYPE *SetFileNameLabel )( _IFileDialog *This, LPCWSTR pszLabel );
	HRESULT ( STDMETHODCALLTYPE *GetResult )( _IFileDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *AddPlace )( _IFileDialog *This, IShellItem *psi, FDAP fdap );
	HRESULT ( STDMETHODCALLTYPE *SetDefaultExtension )( _IFileDialog *This, LPCWSTR pszDefaultExtension );
	HRESULT ( STDMETHODCALLTYPE *Close )( _IFileDialog *This, HRESULT hr );
	HRESULT ( STDMETHODCALLTYPE *SetClientGuid )( _IFileDialog *This, REFGUID guid );
	HRESULT ( STDMETHODCALLTYPE *ClearClientData )( _IFileDialog *This );
	HRESULT ( STDMETHODCALLTYPE *SetFilter )( _IFileDialog *This, IShellItemFilter *pFilter );
} IFileDialogVtbl;

struct _IFileDialog
{
	struct IFileDialogVtbl *lpVtbl;	// This MUST be the first object in the struct.
};
*/
struct _IFileOpenDialog;

typedef struct IFileOpenDialogVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( _IFileOpenDialog *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( _IFileOpenDialog *This );
	ULONG ( STDMETHODCALLTYPE *Release )( _IFileOpenDialog *This );
	HRESULT ( STDMETHODCALLTYPE *Show )( _IFileOpenDialog *This, HWND hwndOwner );
	HRESULT ( STDMETHODCALLTYPE *SetFileTypes )( _IFileOpenDialog *This, UINT cFileTypes, const COMDLG_FILTERSPEC *rgFilterSpec );
	HRESULT ( STDMETHODCALLTYPE *SetFileTypeIndex )( _IFileOpenDialog *This, UINT iFileType );
	HRESULT ( STDMETHODCALLTYPE *GetFileTypeIndex )( _IFileOpenDialog *This, UINT *piFileType );
	HRESULT ( STDMETHODCALLTYPE *Advise )( _IFileOpenDialog *This, IFileDialogEvents *pfde, DWORD *pdwCookie );
	HRESULT ( STDMETHODCALLTYPE *Unadvise )( _IFileOpenDialog *This, DWORD dwCookie );
	HRESULT ( STDMETHODCALLTYPE *SetOptions )( _IFileOpenDialog *This, FILEOPENDIALOGOPTIONS fos );
	HRESULT ( STDMETHODCALLTYPE *GetOptions )( _IFileOpenDialog *This, FILEOPENDIALOGOPTIONS *pfos );
	HRESULT ( STDMETHODCALLTYPE *SetDefaultFolder )( _IFileOpenDialog *This, IShellItem *psi );
	HRESULT ( STDMETHODCALLTYPE *SetFolder )( _IFileOpenDialog *This, IShellItem *psi );
	HRESULT ( STDMETHODCALLTYPE *GetFolder )( _IFileOpenDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *GetCurrentSelection )( _IFileOpenDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *SetFileName )( _IFileOpenDialog *This, LPCWSTR pszName );
	HRESULT ( STDMETHODCALLTYPE *GetFileName )( _IFileOpenDialog *This, LPWSTR *pszName );
	HRESULT ( STDMETHODCALLTYPE *SetTitle )( _IFileOpenDialog *This, LPCWSTR pszTitle );
	HRESULT ( STDMETHODCALLTYPE *SetOkButtonLabel )( _IFileOpenDialog *This, LPCWSTR pszText );
	HRESULT ( STDMETHODCALLTYPE *SetFileNameLabel )( _IFileOpenDialog *This, LPCWSTR pszLabel );
	HRESULT ( STDMETHODCALLTYPE *GetResult )( _IFileOpenDialog *This, IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *AddPlace )( _IFileOpenDialog *This, IShellItem *psi, FDAP fdap );
	HRESULT ( STDMETHODCALLTYPE *SetDefaultExtension )( _IFileOpenDialog *This, LPCWSTR pszDefaultExtension );
	HRESULT ( STDMETHODCALLTYPE *Close )( _IFileOpenDialog *This, HRESULT hr );
	HRESULT ( STDMETHODCALLTYPE *SetClientGuid )( _IFileOpenDialog *This, REFGUID guid );
	HRESULT ( STDMETHODCALLTYPE *ClearClientData )( _IFileOpenDialog *This );
	HRESULT ( STDMETHODCALLTYPE *SetFilter )( _IFileOpenDialog *This, IShellItemFilter *pFilter );
	HRESULT ( STDMETHODCALLTYPE *GetResults )( _IFileOpenDialog *This, IShellItemArray **ppenum );
	HRESULT ( STDMETHODCALLTYPE *GetSelectedItems )( _IFileOpenDialog *This, IShellItemArray **ppsai );
} IFileOpenDialogVtbl;

struct _IFileOpenDialog
{
	struct IFileOpenDialogVtbl *lpVtbl;	// This MUST be the first object in the struct.
};

struct _IShellItem;

typedef struct IShellItemVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( _IShellItem *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( _IShellItem *This );
	ULONG ( STDMETHODCALLTYPE *Release )( _IShellItem *This );
	HRESULT ( STDMETHODCALLTYPE *BindToHandler )( _IShellItem *This, IBindCtx *pbc, REFGUID bhid, REFIID riid, void **ppv );
	HRESULT ( STDMETHODCALLTYPE *GetParent )( _IShellItem *This, _IShellItem **ppsi );
	HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( _IShellItem *This, SIGDN sigdnName, LPWSTR *ppszName );
	HRESULT ( STDMETHODCALLTYPE *GetAttributes )( _IShellItem *This, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs );
	HRESULT ( STDMETHODCALLTYPE *Compare )( _IShellItem *This, _IShellItem *psi, SICHINTF hint, int *piOrder );
} IShellItemVtbl;

struct _IShellItem
{
	struct IShellItemVtbl *lpVtbl;	// This MUST be the first object in the struct.
};

void _BrowseForFolder( HWND hWnd, wchar_t *title, wchar_t **folder );

#endif
