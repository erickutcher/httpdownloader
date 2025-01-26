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

#include "lite_dlls.h"
#include "lite_advapi32.h"

#ifndef ADVAPI32_USE_STATIC_LIB

	pCryptAcquireContextW	_CryptAcquireContextW;
	pCryptGenRandom			_CryptGenRandom;
	pCryptReleaseContext	_CryptReleaseContext;

	//pGetUserNameW			_GetUserNameW;

	pOpenProcessToken		_OpenProcessToken;
	pAdjustTokenPrivileges	_AdjustTokenPrivileges;
	pLookupPrivilegeValueW	_LookupPrivilegeValueW;

	pGetTokenInformation	_GetTokenInformation;

	pAllocateAndInitializeSid	_AllocateAndInitializeSid;
	pCheckTokenMembership	_CheckTokenMembership;

	pFreeSid				_FreeSid;

	pCryptCreateHash		_CryptCreateHash;
	pCryptHashData			_CryptHashData;
	pCryptGetHashParam		_CryptGetHashParam;
	pCryptDestroyHash		_CryptDestroyHash;

	pCryptDestroyKey		_CryptDestroyKey;
	pCryptImportKey			_CryptImportKey;

	HMODULE hModule_advapi32 = NULL;

	unsigned char advapi32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeAdvApi32()
	{
		if ( advapi32_state != ADVAPI32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_advapi32 = LoadLibraryDEMW( L"advapi32.dll" );

		if ( hModule_advapi32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptAcquireContextW, "CryptAcquireContextW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptGenRandom, "CryptGenRandom" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptReleaseContext, "CryptReleaseContext" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_GetUserNameW, "GetUserNameW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_OpenProcessToken, "OpenProcessToken" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_AdjustTokenPrivileges, "AdjustTokenPrivileges" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_LookupPrivilegeValueW, "LookupPrivilegeValueW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_GetTokenInformation, "GetTokenInformation" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_AllocateAndInitializeSid, "AllocateAndInitializeSid" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CheckTokenMembership, "CheckTokenMembership" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_FreeSid, "FreeSid" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptCreateHash, "CryptCreateHash" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptHashData, "CryptHashData" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptGetHashParam, "CryptGetHashParam" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptDestroyHash, "CryptDestroyHash" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptDestroyKey, "CryptDestroyKey" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_advapi32, ( void ** )&_CryptImportKey, "CryptImportKey" ) )

		advapi32_state = ADVAPI32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeAdvApi32()
	{
		if ( advapi32_state != ADVAPI32_STATE_SHUTDOWN )
		{
			advapi32_state = ADVAPI32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_advapi32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
