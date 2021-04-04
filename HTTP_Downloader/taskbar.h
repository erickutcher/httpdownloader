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

#ifndef _TASKBAR_H
#define _TASKBAR_H

#include <shobjidl.h>

//const IID _IID_IUnknown =		{ 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID _IID_ITaskbarList3 =	{ 0xEA1AFB91, 0x9E28, 0x4B86, { 0x90, 0xE9, 0x9E, 0x9F, 0x8A, 0x5E, 0xEF, 0xAF } };

const CLSID _CLSID_TaskbarList = { 0x56FDF344, 0xFD6D, 0x11D0, { 0x95, 0x8A, 0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90 } };

typedef enum THUMBBUTTONFLAGS
{
	THBF_ENABLED =			0x00,
	THBF_DISABLED =			0x01,
	THBF_DISMISSONCLICK =	0x02,
	THBF_NOBACKGROUND =		0x04,
	THBF_HIDDEN =			0x08,
	THBF_NONINTERACTIVE =	0x10
} THUMBBUTTONFLAGS;

typedef enum THUMBBUTTONMASK
{
	THB_BITMAP =	0x01,
	THB_ICON =		0x02,
	THB_TOOLTIP =	0x04,
	THB_FLAGS =		0x08
} THUMBBUTTONMASK;

typedef struct THUMBBUTTON
{
	THUMBBUTTONMASK dwMask;
	UINT iId;
	UINT iBitmap;
	HICON hIcon;
	WCHAR szTip[ 260 ];
	THUMBBUTTONFLAGS dwFlags;
} THUMBBUTTON;

typedef struct THUMBBUTTON *LPTHUMBBUTTON;

typedef enum TBPFLAG
{
	TBPF_NOPROGRESS =		0x00,
	TBPF_INDETERMINATE =	0x01,
	TBPF_NORMAL =			0x02,
	TBPF_ERROR =			0x04,
	TBPF_PAUSED	=			0x08
} TBPFLAG;

struct _ITaskbarList3;

typedef struct ITaskbarList3Vtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( _ITaskbarList3 *This, REFIID riid, void **ppvObject );
	ULONG ( STDMETHODCALLTYPE *AddRef )( _ITaskbarList3 *This );
	ULONG ( STDMETHODCALLTYPE *Release )( _ITaskbarList3 *This );
	HRESULT ( STDMETHODCALLTYPE *HrInit )( _ITaskbarList3 *This );
	HRESULT ( STDMETHODCALLTYPE *AddTab )( _ITaskbarList3 *This, HWND hwnd );
	HRESULT ( STDMETHODCALLTYPE *DeleteTab )( _ITaskbarList3 *This, HWND hwnd );
	HRESULT ( STDMETHODCALLTYPE *ActivateTab )( _ITaskbarList3 *This, HWND hwnd );
	HRESULT ( STDMETHODCALLTYPE *SetActiveAlt )( _ITaskbarList3 *This, HWND hwnd );
	HRESULT ( STDMETHODCALLTYPE *MarkFullscreenWindow )( _ITaskbarList3 *This, HWND hwnd, BOOL fFullscreen );
	HRESULT ( STDMETHODCALLTYPE *SetProgressValue )( _ITaskbarList3 *This, HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal );
	HRESULT ( STDMETHODCALLTYPE *SetProgressState )( _ITaskbarList3 *This, HWND hwnd, TBPFLAG tbpFlags );
	HRESULT ( STDMETHODCALLTYPE *RegisterTab )( _ITaskbarList3 *This, HWND hwndTab, HWND hwndMDI );
	HRESULT ( STDMETHODCALLTYPE *UnregisterTab )( _ITaskbarList3 *This, HWND hwndTab );
	HRESULT ( STDMETHODCALLTYPE *SetTabOrder )( _ITaskbarList3 *This, HWND hwndTab, HWND hwndInsertBefore );
	HRESULT ( STDMETHODCALLTYPE *SetTabActive )( _ITaskbarList3 *This, HWND hwndTab, HWND hwndMDI, DWORD dwReserved );
	HRESULT ( STDMETHODCALLTYPE *ThumbBarAddButtons )( _ITaskbarList3 *This, HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton );
	HRESULT ( STDMETHODCALLTYPE *ThumbBarUpdateButtons )( _ITaskbarList3 *This, HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton );
	HRESULT ( STDMETHODCALLTYPE *ThumbBarSetImageList )( _ITaskbarList3 *This, HWND hwnd, HIMAGELIST himl );
	HRESULT ( STDMETHODCALLTYPE *SetOverlayIcon )( _ITaskbarList3 *This, HWND hwnd, HICON hIcon, LPCWSTR pszDescription );
	HRESULT ( STDMETHODCALLTYPE *SetThumbnailTooltip )( _ITaskbarList3 *This, HWND hwnd, LPCWSTR pszTip );
	HRESULT ( STDMETHODCALLTYPE *SetThumbnailClip )( _ITaskbarList3 *This, HWND hwnd, RECT *prcClip );
} ITaskbarList3Vtbl;

struct _ITaskbarList3
{
	struct ITaskbarList3Vtbl *lpVtbl;	// This MUST be the first object in the struct.
};

#endif
