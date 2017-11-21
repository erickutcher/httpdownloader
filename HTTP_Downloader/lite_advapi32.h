/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2017 Eric Kutcher

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

#ifndef _LITE_ADVAPI32_H
#define _LITE_ADVAPI32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wincrypt.h>

//#define ADVAPI32_USE_STATIC_LIB

#ifdef ADVAPI32_USE_STATIC_LIB

	//__pragma( comment( lib, "advapi32.lib" ) )

	#define _CryptAcquireContextW	CryptAcquireContextW
	#define _CryptGenRandom			CryptGenRandom
	#define _CryptReleaseContext	CryptReleaseContext

	//#define _GetUserNameW			GetUserNameW

	#define _OpenProcessToken		OpenProcessToken
	#define _AdjustTokenPrivileges	AdjustTokenPrivileges
	#define _LookupPrivilegeValueW	LookupPrivilegeValueW

	#define _GetTokenInformation	GetTokenInformation

	#define _AllocateAndInitializeSid	AllocateAndInitializeSid
	#define _CheckTokenMembership	CheckTokenMembership

	#define _FreeSid				FreeSid

	#define _CryptCreateHash			CryptCreateHash
	#define _CryptHashData				CryptHashData
	#define _CryptGetHashParam			CryptGetHashParam
	#define _CryptDestroyHash			CryptDestroyHash

	#define _CryptDestroyKey		CryptDestroyKey
	#define _CryptImportKey			CryptImportKey

#else

	#define ADVAPI32_STATE_SHUTDOWN		0
	#define ADVAPI32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pCryptAcquireContextW )( HCRYPTPROV *phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags );
	typedef BOOL ( WINAPI *pCryptGenRandom )( HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer );
	typedef BOOL ( WINAPI *pCryptReleaseContext )( HCRYPTPROV hProv, DWORD dwFlags );

	//typedef BOOL ( WINAPI *pGetUserNameW )( LPTSTR lpBuffer, LPDWORD lpnSize );

	typedef BOOL ( WINAPI *pOpenProcessToken )( HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle );
	typedef BOOL ( WINAPI *pAdjustTokenPrivileges )( HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength );
	typedef BOOL ( WINAPI *pLookupPrivilegeValueW )( LPCTSTR lpSystemName, LPCTSTR lpName, PLUID lpLuid );

	typedef BOOL ( WINAPI *pGetTokenInformation )( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength );

	typedef BOOL ( WINAPI *pAllocateAndInitializeSid )( PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD dwSubAuthority0, DWORD dwSubAuthority1, DWORD dwSubAuthority2, DWORD dwSubAuthority3, DWORD dwSubAuthority4, DWORD dwSubAuthority5, DWORD dwSubAuthority6, DWORD dwSubAuthority7, PSID *pSid );
	typedef BOOL ( WINAPI *pCheckTokenMembership )( HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember );

	typedef PVOID  ( WINAPI *pFreeSid )( PSID pSid );

	typedef BOOL ( WINAPI *pCryptCreateHash )( HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH *phHash );
	typedef BOOL ( WINAPI *pCryptHashData )( HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags );
	typedef BOOL ( WINAPI *pCryptGetHashParam )( HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD *pdwDataLen, DWORD dwFlags );
	typedef BOOL ( WINAPI *pCryptDestroyHash )( HCRYPTHASH hHash );

	typedef BOOL ( WINAPI *pCryptDestroyKey )( HCRYPTKEY hKey );
	typedef BOOL ( WINAPI *pCryptImportKey )( HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey );

	extern pCryptAcquireContextW	_CryptAcquireContextW;
	extern pCryptGenRandom			_CryptGenRandom;
	extern pCryptReleaseContext		_CryptReleaseContext;

	//extern pGetUserNameW			_GetUserNameW;

	extern pOpenProcessToken		_OpenProcessToken;
	extern pAdjustTokenPrivileges	_AdjustTokenPrivileges;
	extern pLookupPrivilegeValueW	_LookupPrivilegeValueW;

	extern pGetTokenInformation		_GetTokenInformation;

	extern pAllocateAndInitializeSid	_AllocateAndInitializeSid;
	extern pCheckTokenMembership	_CheckTokenMembership;

	extern pFreeSid					_FreeSid;

	extern pCryptCreateHash			_CryptCreateHash;
	extern pCryptHashData			_CryptHashData;
	extern pCryptGetHashParam		_CryptGetHashParam;
	extern pCryptDestroyHash		_CryptDestroyHash;

	extern pCryptDestroyKey			_CryptDestroyKey;
	extern pCryptImportKey			_CryptImportKey;

	extern unsigned char advapi32_state;

	bool InitializeAdvApi32();
	bool UnInitializeAdvApi32();

#endif

#endif
