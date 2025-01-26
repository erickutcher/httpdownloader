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

#ifndef _DRAG_AND_DROP
#define _DRAG_AND_DROP

#include <oleidl.h>

//const IID _IID_IUnknown =	 { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID _IID_IDataObject = { 0x0000010e, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID _IID_IDropSource = { 0x00000121, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID _IID_IDropTarget = { 0x00000122, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

typedef struct IDropTargetVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IDropTarget *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( IDropTarget *This );
	ULONG ( STDMETHODCALLTYPE *Release )( IDropTarget *This );
	HRESULT ( STDMETHODCALLTYPE *DragEnter )( IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );
	HRESULT ( STDMETHODCALLTYPE *DragOver )( IDropTarget *This, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );
	HRESULT ( STDMETHODCALLTYPE *DragLeave )( IDropTarget *This );
	HRESULT ( STDMETHODCALLTYPE *Drop )( IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );
} IDropTargetVtbl;

typedef struct _IDropTarget
{
	struct IDropTargetVtbl *lpVtbl;	// This MUST be the first object in the struct.

	HWND			m_hWnd;
	LONG			m_lRefCount;
	UINT			m_ClipFormat;
} _IDropTarget;

//

typedef struct IDropSourceVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IDropSource *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( IDropSource *This );
	ULONG ( STDMETHODCALLTYPE *Release )( IDropSource *This );
	HRESULT ( STDMETHODCALLTYPE *QueryContinueDrag )( IDropSource *This, BOOL fEscapePressed, DWORD grfKeyState );
	HRESULT ( STDMETHODCALLTYPE *GiveFeedback )( IDropSource *This, DWORD dwEffect );
} IDropSourceVtbl;

typedef struct _IDropSource
{
	struct IDropSourceVtbl *lpVtbl;	// This MUST be the first object in the struct.

	HWND			m_hWnd;
	LONG			m_lRefCount;
} _IDropSource;

//

typedef struct IDataObjectVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( IDataObject *This, REFIID riid,  void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( IDataObject *This );
	ULONG ( STDMETHODCALLTYPE *Release )( IDataObject *This );
	HRESULT ( STDMETHODCALLTYPE *GetData )( IDataObject *This, FORMATETC *pformatetcIn, STGMEDIUM *pmedium );
	HRESULT ( STDMETHODCALLTYPE *GetDataHere )( IDataObject *This, FORMATETC *pformatetc, STGMEDIUM *pmedium );
	HRESULT ( STDMETHODCALLTYPE *QueryGetData )( IDataObject *This, FORMATETC *pformatetc );
	HRESULT ( STDMETHODCALLTYPE *GetCanonicalFormatEtc )( IDataObject *This, FORMATETC *pformatectIn, FORMATETC *pformatetcOut );
	HRESULT ( STDMETHODCALLTYPE *SetData )( IDataObject *This, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease );
	HRESULT ( STDMETHODCALLTYPE *EnumFormatEtc )( IDataObject *This, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc );
	HRESULT ( STDMETHODCALLTYPE *DAdvise )( IDataObject *This, FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection );
	HRESULT ( STDMETHODCALLTYPE *DUnadvise )( IDataObject *This, DWORD dwConnection );
	HRESULT ( STDMETHODCALLTYPE *EnumDAdvise )( IDataObject *This, IEnumSTATDATA **ppenumAdvise );
} IDataObjectVtbl;

typedef struct _IDataObject
{
	struct IDataObjectVtbl *lpVtbl;	// This MUST be the first object in the struct.

	STGMEDIUM		*m_pStgMedium;
	FORMATETC		*m_pFormatEtc;
	LONG			m_lRefCount;
	int				m_nNumFormats;
} _IDataObject;

//

void RegisterDropWindow( HWND hWnd, IDropTarget **DropTarget );
void UnregisterDropWindow( HWND hWnd, IDropTarget *DropTarget );

_IDropSource *Create_IDropSource( HWND hWnd );
_IDataObject *Create_IDataObject( FORMATETC *fmtetc, STGMEDIUM *stgmed, int count );

#endif
