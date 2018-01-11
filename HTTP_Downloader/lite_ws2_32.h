/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#ifndef _LITE_WINSOCK_H
#define _LITE_WINSOCK_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

//#define WS2_32_USE_STATIC_LIB

#define WS2_32_STATE_SHUTDOWN		0
#define WS2_32_STATE_RUNNING		1

#ifdef WS2_32_USE_STATIC_LIB

	//__pragma( comment( lib, "ws2_32.lib" ) )

	#define _WSAStartup		WSAStartup
	#define _WSACleanup		WSACleanup

	#define _WSAWaitForMultipleEvents	WSAWaitForMultipleEvents
	#define _WSACreateEvent	WSACreateEvent
	#define _WSASetEvent	WSASetEvent
	#define _WSAResetEvent	WSAResetEvent
	//#define _WSAEventSelect WSAEventSelect
	#define _WSACloseEvent	WSACloseEvent

	#define _WSASocketW		WSASocketW
	#define _WSAConnect		WSAConnect

	#define _WSAAddressToStringW	WSAAddressToStringW
	#define _FreeAddrInfoW	FreeAddrInfoW
	#define _GetAddrInfoW	GetAddrInfoW
	#define _GetNameInfoW	GetNameInfoW

	#define _WSAGetLastError	WSAGetLastError

	#define _WSAIoctl		WSAIoctl

	#define _WSASend		WSASend
	#define _WSARecv		WSARecv

	#define _socket			socket
	#define _connect		connect
	#define _listen			listen
	#define _accept			accept
	#define _bind			bind
	#define _shutdown		shutdown
	#define _closesocket	closesocket

	#define _setsockopt		setsockopt

	#define _send			send
	#define _recv			recv

	#define _getaddrinfo	getaddrinfo
	#define _freeaddrinfo	freeaddrinfo
	#define _getpeername	getpeername

	#define _inet_ntoa		inet_ntoa

	#define _htonl			htonl

#else

	typedef int ( WSAAPI *pWSAStartup )( WORD wVersionRequested, LPWSADATA lpWSAData );
	typedef int ( WSAAPI *pWSACleanup )( void );

	typedef DWORD ( WSAAPI *pWSAWaitForMultipleEvents )( DWORD cEvents, const WSAEVENT *lphEvents, BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable );
	typedef WSAEVENT ( WSAAPI *pWSACreateEvent )( void );
	typedef BOOL ( WSAAPI *pWSASetEvent )( WSAEVENT hEvent );
	typedef BOOL ( WSAAPI *pWSAResetEvent )( WSAEVENT hEvent );
	//typedef int ( WSAAPI *pWSAEventSelect )( SOCKET s, WSAEVENT hEventObject, long lNetworkEvents );
	typedef BOOL ( WSAAPI *pWSACloseEvent )( WSAEVENT hEvent );

	typedef SOCKET ( WSAAPI *pWSASocketW )( int af, int type, int protocol, LPWSAPROTOCOL_INFO lpProtocolInfo, GROUP g, DWORD dwFlags );
	typedef int ( WSAAPI *pWSAConnect )( SOCKET s, const struct sockaddr *name, int namelen, LPWSABUF lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS );

	typedef INT ( WSAAPI *pWSAAddressToStringW )( LPSOCKADDR lpsaAddress, DWORD dwAddressLength, LPWSAPROTOCOL_INFO lpProtocolInfo, LPTSTR lpszAddressString, LPDWORD lpdwAddressStringLength );
	typedef void ( WSAAPI *pFreeAddrInfoW )( PADDRINFOW pAddrInfo );
	typedef int ( WSAAPI *pGetAddrInfoW )( PCWSTR pNodeName, PCWSTR pServiceName, const ADDRINFOW *pHints, PADDRINFOW *ppResult );
	typedef int ( WSAAPI *pGetNameInfoW )( const SOCKADDR *pSockaddr, socklen_t SockaddrLength, PWCHAR pNodeBuffer, DWORD NodeBufferSize, PWCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags );

	typedef int ( WSAAPI *pWSAGetLastError )( void );

	typedef int ( WSAAPI *pWSAIoctl )( SOCKET s, DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer, LPVOID lpvOutBuffer, DWORD cbOutBuffer, LPDWORD lpcbBytesReturned, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );

	typedef int ( WSAAPI *pWSASend )( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );
	typedef int ( WSAAPI *pWSARecv )( SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );

	typedef SOCKET ( WSAAPI *psocket )( int af, int type, int protocol );
	typedef int ( WSAAPI *pconnect )( SOCKET s, const struct sockaddr *name, int namelen );
	typedef int ( WSAAPI *plisten )( SOCKET s, int backlog );
	typedef SOCKET ( WSAAPI *paccept )( SOCKET s, struct sockaddr *addr, int *addrlen );
	typedef int ( WSAAPI *pbind )( SOCKET s, const struct sockaddr *name, int namelen );
	typedef int ( WSAAPI *pshutdown )( SOCKET s, int how );
	typedef int ( WSAAPI *pclosesocket )( SOCKET s );

	typedef int ( WSAAPI *psetsockopt )( SOCKET s, int level, int optname, const char *optval, int optlen );

	typedef int ( WSAAPI *psend )( SOCKET s, const char *buf, int len, int flags );
	typedef int ( WSAAPI *precv )( SOCKET s, const char *buf, int len, int flags );

	typedef int ( WSAAPI *pgetaddrinfo )( PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA *pHints, PADDRINFOA *ppResult );
	typedef void ( WSAAPI *pfreeaddrinfo )( struct addrinfo *ai );
	typedef int ( WSAAPI *pgetpeername )( SOCKET s, struct sockaddr *name, int *namelen );

	typedef char *FAR ( WSAAPI *pinet_ntoa )( struct in_addr in );

	typedef u_long ( WSAAPI *phtonl )( u_long hostlong );

	extern pWSAStartup		_WSAStartup;
	extern pWSACleanup		_WSACleanup;

	extern pWSAWaitForMultipleEvents	_WSAWaitForMultipleEvents;
	extern pWSACreateEvent	_WSACreateEvent;
	extern pWSASetEvent		_WSASetEvent;
	extern pWSAResetEvent	_WSAResetEvent;
	//extern pWSAEventSelect	_WSAEventSelect;
	extern pWSACloseEvent	_WSACloseEvent;

	extern pWSASocketW		_WSASocketW;
	extern pWSAConnect		_WSAConnect;

	extern pWSAAddressToStringW		_WSAAddressToStringW;
	extern pFreeAddrInfoW	_FreeAddrInfoW;
	extern pGetAddrInfoW	_GetAddrInfoW;
	extern pGetNameInfoW	_GetNameInfoW;

	extern pWSAGetLastError	_WSAGetLastError;

	extern pWSAIoctl		_WSAIoctl;

	extern pWSASend			_WSASend;
	extern pWSARecv			_WSARecv;

	extern psocket			_socket;
	extern pconnect			_connect;
	extern plisten			_listen;
	extern paccept			_accept;
	extern pbind			_bind;
	extern pshutdown		_shutdown;
	extern pclosesocket		_closesocket;

	extern psetsockopt		_setsockopt;

	extern psend			_send;
	extern precv			_recv;

	extern pgetaddrinfo		_getaddrinfo;
	extern pfreeaddrinfo	_freeaddrinfo;
	extern pgetpeername		_getpeername;

	extern pinet_ntoa		_inet_ntoa;

	extern phtonl			_htonl;

	bool InitializeWS2_32();
	bool UnInitializeWS2_32();

#endif

void StartWS2_32();
void EndWS2_32();

extern unsigned char ws2_32_state;

#endif
