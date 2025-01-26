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

#ifndef _LITE_SHELL32_H
#define _LITE_SHELL32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>
#include <initguid.h>
#include <knownfolders.h>
#include <shlobj.h>

//#define SHELL32_USE_STATIC_LIB

#ifdef SHELL32_USE_STATIC_LIB

	//__pragma( comment( lib, "shell32.lib" ) )
	//__pragma( comment( lib, "shlwapi.lib" ) )

	#include <shlwapi.h>

	#define _Shell_NotifyIconW	Shell_NotifyIconW
	#define _ShellExecuteW		ShellExecuteW

	#define _StrChrA			StrChrA
	#define _StrChrW			StrChrW
	#define _StrStrA			StrStrA
	#define _StrStrW			StrStrW
	#define _StrStrIA			StrStrIA
	#define _StrStrIW			StrStrIW

	#define _StrCmpNA			StrCmpNA
	#define _StrCmpNW			StrCmpNW
	#define _StrCmpNIA			StrCmpNIA

	#define _StrCmpNIW			StrCmpNIW

	#define _SHBrowseForFolderW	SHBrowseForFolderW
	#define _SHGetPathFromIDListW	SHGetPathFromIDListW

	#define _SHGetFileInfoW		SHGetFileInfoW
	#define _SHGetFolderPathW	SHGetFolderPathW
	#define _SHGetKnownFolderPath	SHGetKnownFolderPath;

	#define _SHFileOperationW	SHFileOperationW;

	#define _SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems
	#define _ILCreateFromPathW	ILCreateFromPathW
	//#define _ILFree				ILFree

	#define	_CommandLineToArgvW	CommandLineToArgvW

	//#define _DragAcceptFiles	DragAcceptFiles
	#define _DragQueryFileW		DragQueryFileW
	//#define _DragFinish			DragFinish

	#define _SHCreateStdEnumFmtEtc	SHCreateStdEnumFmtEtc

	#define _ExtractIconExW		ExtractIconExW

#else

	#define SHELL32_STATE_SHUTDOWN		0
	#define SHELL32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pShell_NotifyIconW )( DWORD dwMessage, PNOTIFYICONDATA lpdata );
	typedef HINSTANCE ( WINAPI *pShellExecuteW )( HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd );

	typedef PSTR ( WINAPI *pStrChrA )( PSTR pszStart, CHAR wMatch );
	typedef PWSTR ( WINAPI *pStrChrW )( PWSTR pszStart, WCHAR wMatch );
	typedef PSTR ( WINAPI *pStrStrA )( PSTR pszFirst, PCSTR pszSrch );
	typedef PWSTR ( WINAPI *pStrStrW )( PWSTR pszFirst, PCWSTR pszSrch );
	typedef PSTR ( WINAPI *pStrStrIA )( PSTR pszFirst, PCSTR pszSrch );
	typedef PSTR ( WINAPI *pStrStrIW )( PWSTR pszFirst, PCWSTR pszSrch );

	typedef int ( WINAPI *pStrCmpNA )( PCSTR psz1, PCSTR psz2, int nChar );
	typedef int ( WINAPI *pStrCmpNW )( PCWSTR psz1, PCWSTR psz2, int nChar );
	typedef int ( WINAPI *pStrCmpNIA )( PCSTR psz1, PCSTR psz2, int nChar );

	typedef int ( WINAPI *pStrCmpNIW )( PCTSTR psz1, PCTSTR psz2, int nChar );

	typedef PIDLIST_ABSOLUTE ( WINAPI *pSHBrowseForFolderW )( LPBROWSEINFO lpbi );
	typedef BOOL ( WINAPI *pSHGetPathFromIDListW )( PCIDLIST_ABSOLUTE pidl, LPTSTR pszPath );

	typedef DWORD_PTR ( WINAPI *pSHGetFileInfoW )( LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO *psfi, UINT cbFileInfo, UINT uFlags );
	typedef HRESULT ( WINAPI *pSHGetFolderPathW )( HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath );
	typedef HRESULT ( WINAPI *pSHGetKnownFolderPath )( REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath );

	typedef int ( WINAPI *pSHFileOperationW )( LPSHFILEOPSTRUCTW lpFileOp );

	typedef HRESULT ( WINAPI *pSHOpenFolderAndSelectItems )( PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD dwFlags ); // Returns an HRESULT but is documented/defined as SHSTDAPI.
	typedef PIDLIST_ABSOLUTE ( WINAPI *pILCreateFromPathW )( PCWSTR pszPath );
	//typedef void ( WINAPI *pILFree )( PIDLIST_RELATIVE pidl );

	typedef LPWSTR * ( WINAPI *pCommandLineToArgvW )( LPCWSTR lpCmdLine, int *pNumArgs );

	//typedef VOID ( WINAPI *pDragAcceptFiles )( HWND hWnd, BOOL fAccept );
	typedef UINT ( WINAPI *pDragQueryFileW )( HDROP hDrop, UINT iFile, LPTSTR lpszFile, UINT cch );
	//typedef VOID ( WINAPI *pDragFinish )( HDROP hDrop );

	typedef HRESULT ( WINAPI *pSHCreateStdEnumFmtEtc )( UINT cfmt, const FORMATETC afmt[], IEnumFORMATETC **ppenumFormatEtc );

	typedef UINT ( WINAPI *pExtractIconExW )( LPCWSTR lpszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons );

	extern pShell_NotifyIconW	_Shell_NotifyIconW;
	extern pShellExecuteW		_ShellExecuteW;

	extern pStrChrA				_StrChrA;
	extern pStrChrW				_StrChrW;
	extern pStrStrA				_StrStrA;
	extern pStrStrW				_StrStrW;
	extern pStrStrIA			_StrStrIA;
	extern pStrStrIW			_StrStrIW;

	extern pStrCmpNA			_StrCmpNA;
	extern pStrCmpNW			_StrCmpNW;
	extern pStrCmpNIA			_StrCmpNIA;

	extern pStrCmpNIW			_StrCmpNIW;

	extern pSHBrowseForFolderW	_SHBrowseForFolderW;
	extern pSHGetPathFromIDListW	_SHGetPathFromIDListW;

	extern pSHGetFileInfoW		_SHGetFileInfoW;
	extern pSHGetFolderPathW	_SHGetFolderPathW;
	extern pSHGetKnownFolderPath	_SHGetKnownFolderPath;

	extern pSHFileOperationW	_SHFileOperationW;

	extern pSHOpenFolderAndSelectItems	_SHOpenFolderAndSelectItems;
	extern pILCreateFromPathW	_ILCreateFromPathW;
	//extern pILFree				_ILFree;

	extern pCommandLineToArgvW	_CommandLineToArgvW;

	//extern pDragAcceptFiles		_DragAcceptFiles;
	extern pDragQueryFileW		_DragQueryFileW;
	//extern pDragFinish			_DragFinish;

	extern pSHCreateStdEnumFmtEtc	_SHCreateStdEnumFmtEtc;

	extern pExtractIconExW		_ExtractIconExW;

	extern unsigned char shell32_state;

	bool InitializeShell32();
	bool UnInitializeShell32();

#endif

extern HMODULE hModule_shell32;

/*const GUID _FOLDERID_Desktop = { 0xb4bfcc3a, 0xdb2c, 0x424c, { 0xb0, 0x29, 0x7f, 0xe9, 0x9a, 0x87, 0xc6, 0x41 } };
const GUID _FOLDERID_Documents = { 0xfdd39ad0, 0x238f, 0x46af, { 0xad, 0xb4, 0x6c, 0x85, 0x48, 0x03, 0x69, 0xc7 } };

const GUID _FOLDERID_Downloads = { 0x374de290, 0x123f, 0x4565, { 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b } };

const GUID _FOLDERID_Music = { 0x4bd8d571, 0x6d19, 0x48d3, { 0xbe, 0x97, 0x42, 0x22, 0x20, 0x08, 0x0e, 0x43 } };
const GUID _FOLDERID_Pictures = { 0x33e28130, 0x4e1e, 0x4676, { 0x83, 0x5a, 0x98, 0x39, 0x5c, 0x3b, 0xc3, 0xbb } };
const GUID _FOLDERID_Videos = { 0x18989b1d, 0x99b5, 0x455b, { 0x84, 0x1c, 0xab, 0x7c, 0x74, 0xe4, 0xdd, 0xfc } };*/

int _StrCmpA( PCSTR psz1, PCSTR psz2 );
int _StrCmpW( PCWSTR psz1, PCWSTR psz2 );

#endif
