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

#ifndef _LITE_RPCRT4_H
#define _LITE_RPCRT4_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <rpc.h>

//#define RPCRT4_USE_STATIC_LIB

#ifdef RPCRT4_USE_STATIC_LIB

	//__pragma( comment( lib, "rpcrt4.lib" ) )

	#define _RpcStringFreeW		RpcStringFreeW
	#define _UuidToStringW		UuidToStringW
	#define _UuidCreate			UuidCreate

#else

	#define RPCRT4_STATE_SHUTDOWN		0
	#define RPCRT4_STATE_RUNNING		1

	typedef RPC_STATUS ( RPC_ENTRY *pRpcStringFreeW )( RPC_WSTR __RPC_FAR *String );
	typedef RPC_STATUS ( RPC_ENTRY *pUuidToStringW )( UUID __RPC_FAR *Uuid, RPC_WSTR __RPC_FAR *StringUuid );
	typedef RPC_STATUS ( RPC_ENTRY *pUuidCreate )( UUID __RPC_FAR *Uuid );

	extern pRpcStringFreeW		_RpcStringFreeW;
	extern pUuidToStringW		_UuidToStringW;
	extern pUuidCreate			_UuidCreate;

	extern unsigned char rpcrt4_state;

	bool InitializeRpcRt4();
	bool UnInitializeRpcRt4();

#endif

#endif
