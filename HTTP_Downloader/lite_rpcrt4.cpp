/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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
#include "lite_rpcrt4.h"

#ifndef RPCRT4_USE_STATIC_LIB

	pRpcStringFreeW		_RpcStringFreeW;
	pUuidToStringW		_UuidToStringW;
	pUuidCreate			_UuidCreate;

	HMODULE hModule_rpcrt4 = NULL;

	unsigned char rpcrt4_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeRpcRt4()
	{
		if ( rpcrt4_state != RPCRT4_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_rpcrt4 = LoadLibraryDEMW( L"rpcrt4.dll" );

		if ( hModule_rpcrt4 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_rpcrt4, ( void ** )&_RpcStringFreeW, "RpcStringFreeW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_rpcrt4, ( void ** )&_UuidToStringW, "UuidToStringW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_rpcrt4, ( void ** )&_UuidCreate, "UuidCreate" ) )

		rpcrt4_state = RPCRT4_STATE_RUNNING;

		return true;
	}

	bool UnInitializeRpcRt4()
	{
		if ( rpcrt4_state != RPCRT4_STATE_SHUTDOWN )
		{
			rpcrt4_state = RPCRT4_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_rpcrt4 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
