/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2024 Eric Kutcher

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

#ifndef _LITE_PSFTP_H
#define _LITE_PSFTP_H

#include <ws2def.h>

#define SSH_FXP_INIT                              1		// 0x1
#define SSH_FXP_VERSION                           2		// 0x2
#define SSH_FXP_OPEN                              3		// 0x3
#define SSH_FXP_CLOSE                             4		// 0x4
#define SSH_FXP_READ                              5		// 0x5
#define SSH_FXP_WRITE                             6		// 0x6
#define SSH_FXP_LSTAT                             7		// 0x7
#define SSH_FXP_FSTAT                             8		// 0x8
#define SSH_FXP_SETSTAT                           9		// 0x9
#define SSH_FXP_FSETSTAT                          10	// 0xa
#define SSH_FXP_OPENDIR                           11	// 0xb
#define SSH_FXP_READDIR                           12	// 0xc
#define SSH_FXP_REMOVE                            13	// 0xd
#define SSH_FXP_MKDIR                             14	// 0xe
#define SSH_FXP_RMDIR                             15	// 0xf
#define SSH_FXP_REALPATH                          16	// 0x10
#define SSH_FXP_STAT                              17	// 0x11
#define SSH_FXP_RENAME                            18	// 0x12
#define SSH_FXP_STATUS                            101	// 0x65
#define SSH_FXP_HANDLE                            102	// 0x66
#define SSH_FXP_DATA                              103	// 0x67
#define SSH_FXP_NAME                              104	// 0x68
#define SSH_FXP_ATTRS                             105	// 0x69
#define SSH_FXP_EXTENDED                          200	// 0xc8
#define SSH_FXP_EXTENDED_REPLY                    201	// 0xc9

#define SSH_STATUS_NONE						0x00000000
#define SSH_STATUS_CLEANUP					0x00000001
#define SSH_STATUS_WRITE					0x00000002
#define SSH_STATUS_AUTHENTICATE				0x00000004
#define SSH_STATUS_KEY_NOT_FOUND			0x00000008
#define SSH_STATUS_KEY_MISMATCH				0x00000010
#define SSH_STATUS_INITIALIZED				0x00000020
#define SSH_STATUS_INITIALIZED_FILE_HANDLE	0x00000040
#define SSH_STATUS_USER_CLEANUP				0x00000080
#define SSH_STATUS_BACKEND_CLOSED			0x00000100

typedef void SSH;

struct FILE_ATTRIBUTES
{
	unsigned long long size;
    unsigned long mtime;
};

//#define PSFTP_USE_STATIC_LIB

#ifdef PSFTP_USE_STATIC_LIB

	//__pragma( comment( lib, "psftp.lib" ) )

	#define _SFTP_InitGSSAPI				SFTP_InitGSSAPI
	#define _SFTP_UninitGSSAPI				SFTP_UninitGSSAPI
	#define _SFTP_SetConfigInfo				SFTP_SetConfigInfo
	#define _SFTP_SetAlgorithmPriorities	SFTP_SetAlgorithmPriorities
	#define _SFTP_CreateSSHHandle			SFTP_CreateSSHHandle
	#define _SFTP_BackendFree				SFTP_BackendFree
	#define _SFTP_FreeSSHHandle				SFTP_FreeSSHHandle
	#define _SFTP_CheckCallbacks			SFTP_CheckCallbacks
	#define _SFTP_RunCallbacks				SFTP_RunCallbacks
	#define _SFTP_GetKeyInfo				SFTP_GetKeyInfo
	#define _SFTP_ProcessWriteRequest		SFTP_ProcessWriteRequest
	#define _SFTP_CheckInitStatus			SFTP_CheckInitStatus
	#define _SFTP_InitSendVersion			SFTP_InitSendVersion
	#define _SFTP_ProcessGetRequestBuffer	SFTP_ProcessGetRequestBuffer
	#define _SFTP_GetStatus					SFTP_GetStatus
	#define _SFTP_SetStatus					SFTP_SetStatus
	#define _SFTP_CheckCallbackStatus		SFTP_CheckCallbackStatus
	#define _SFTP_GetRequestPacket			SFTP_GetRequestPacket
	#define _SFTP_GetPacketInfo				SFTP_GetPacketInfo
	#define _SFTP_ResetPacketInfo			SFTP_ResetPacketInfo
	#define _SFTP_PrepareRequestPacket		SFTP_PrepareRequestPacket
	#define _SFTP_GetRequestPacketType		SFTP_GetRequestPacketType
	#define _SFTP_ProcessVersion			SFTP_ProcessVersion
	#define _SFTP_GetAttributes				SFTP_GetAttributes
	#define _SFTP_ProcessAttributes			SFTP_ProcessAttributes
	#define _SFTP_GetHandle					SFTP_GetHandle
	#define _SFTP_ProcessDownloadHandle		SFTP_ProcessDownloadHandle
	#define _SFTP_DownloadInit				SFTP_DownloadInit
	#define _SFTP_DownloadPrepareData		SFTP_DownloadPrepareData
	#define _SFTP_DownloadData				SFTP_DownloadData
	#define _SFTP_IsDownloadDone			SFTP_IsDownloadDone
	#define _SFTP_DownloadQueue				SFTP_DownloadQueue
	#define _SFTP_DownloadClose				SFTP_DownloadClose
	#define _SFTP_DownloadCleanupPacket		SFTP_DownloadCleanupPacket
	#define _SFTP_DownloadCleanupTransfer	SFTP_DownloadCleanupTransfer
	#define _SFTP_CheckBackendStatus		SFTP_CheckBackendStatus
	#define _SFTP_BackendClose				SFTP_BackendClose
	#define _SFTP_FreeDownloadData			SFTP_FreeDownloadData

#else

	#define PSFTP_STATE_SHUTDOWN	0
	#define PSFTP_STATE_RUNNING		1

	typedef VOID ( WINAPIV *pSFTP_InitGSSAPI )();
	typedef VOID ( WINAPIV *pSFTP_UninitGSSAPI )();
	typedef VOID ( WINAPIV *pSFTP_SetConfigInfo )( unsigned char info, unsigned long value );
	typedef VOID ( WINAPIV *pSFTP_SetAlgorithmPriorities )( unsigned char algorithm, unsigned char priority_list[], unsigned char priority_list_length );
	typedef PVOID ( WINAPIV *pSFTP_CreateSSHHandle )( wchar_t *canonname, char *username, char *password, char *key_info, char *private_key_file_path, WSABUF *wsabuf );
	typedef VOID ( WINAPIV *pSFTP_BackendFree )( SSH *ssh );
	typedef VOID ( WINAPIV *pSFTP_FreeSSHHandle )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_CheckCallbacks )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_RunCallbacks )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_GetKeyInfo )( SSH *ssh, char **algorithm, int *key_size, char **md5_fingerprint, char **sha256_fingerprint );
	typedef INT ( WINAPIV *pSFTP_ProcessWriteRequest )( SSH *ssh, DWORD io_size );
	typedef INT ( WINAPIV *pSFTP_CheckInitStatus )( SSH *ssh );
	typedef VOID ( WINAPIV *pSFTP_InitSendVersion )( SSH *ssh );
	typedef VOID ( WINAPIV *pSFTP_ProcessGetRequestBuffer )( SSH *ssh, char *buffer, DWORD io_size );
	typedef INT ( WINAPIV *pSFTP_GetStatus )( SSH *ssh );
	typedef VOID ( WINAPIV *pSFTP_SetStatus )( SSH *ssh, int status );
	typedef INT ( WINAPIV *pSFTP_CheckCallbackStatus )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_GetRequestPacket )( SSH *ssh, char type );
	typedef INT ( WINAPIV *pSFTP_GetPacketInfo )( SSH *ssh, char type, char **buf, size_t *len );
	typedef INT ( WINAPIV *pSFTP_ResetPacketInfo )( SSH *ssh, char type );
	typedef INT ( WINAPIV *pSFTP_PrepareRequestPacket )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_GetRequestPacketType )( SSH *ssh, int *packet_type );
	typedef INT ( WINAPIV *pSFTP_ProcessVersion )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_GetAttributes )( SSH *ssh, char *fname );
	typedef INT ( WINAPIV *pSFTP_ProcessAttributes )( SSH *ssh, FILE_ATTRIBUTES *file_attributes );
	typedef INT ( WINAPIV *pSFTP_GetHandle )( SSH *ssh, char *path );
	typedef INT ( WINAPIV *pSFTP_ProcessDownloadHandle )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadInit )( SSH *ssh, unsigned long long offset, unsigned long long filesize );
	typedef INT ( WINAPIV *pSFTP_DownloadPrepareData )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadData )( SSH *ssh, char **buffer, DWORD *io_size );
	typedef INT ( WINAPIV *pSFTP_IsDownloadDone )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadQueue )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadClose )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadCleanupPacket )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_DownloadCleanupTransfer )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_CheckBackendStatus )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_BackendClose )( SSH *ssh );
	typedef INT ( WINAPIV *pSFTP_FreeDownloadData )( char *buffer );

	extern pSFTP_InitGSSAPI					_SFTP_InitGSSAPI;
	extern pSFTP_UninitGSSAPI				_SFTP_UninitGSSAPI;
	extern pSFTP_SetConfigInfo				_SFTP_SetConfigInfo;
	extern pSFTP_SetAlgorithmPriorities		_SFTP_SetAlgorithmPriorities;
	extern pSFTP_CreateSSHHandle			_SFTP_CreateSSHHandle;
	extern pSFTP_BackendFree				_SFTP_BackendFree;
	extern pSFTP_FreeSSHHandle				_SFTP_FreeSSHHandle;
	extern pSFTP_CheckCallbacks				_SFTP_CheckCallbacks;
	extern pSFTP_RunCallbacks				_SFTP_RunCallbacks;
	extern pSFTP_GetKeyInfo					_SFTP_GetKeyInfo;
	extern pSFTP_ProcessWriteRequest		_SFTP_ProcessWriteRequest;
	extern pSFTP_CheckInitStatus			_SFTP_CheckInitStatus;
	extern pSFTP_InitSendVersion			_SFTP_InitSendVersion;
	extern pSFTP_ProcessGetRequestBuffer	_SFTP_ProcessGetRequestBuffer;
	extern pSFTP_GetStatus					_SFTP_GetStatus;
	extern pSFTP_SetStatus					_SFTP_SetStatus;
	extern pSFTP_CheckCallbackStatus		_SFTP_CheckCallbackStatus;
	extern pSFTP_GetRequestPacket			_SFTP_GetRequestPacket;
	extern pSFTP_GetPacketInfo				_SFTP_GetPacketInfo;
	extern pSFTP_ResetPacketInfo			_SFTP_ResetPacketInfo;
	extern pSFTP_PrepareRequestPacket		_SFTP_PrepareRequestPacket;
	extern pSFTP_GetRequestPacketType		_SFTP_GetRequestPacketType;
	extern pSFTP_ProcessVersion				_SFTP_ProcessVersion;
	extern pSFTP_GetAttributes				_SFTP_GetAttributes;
	extern pSFTP_ProcessAttributes			_SFTP_ProcessAttributes;
	extern pSFTP_GetHandle					_SFTP_GetHandle;
	extern pSFTP_ProcessDownloadHandle		_SFTP_ProcessDownloadHandle;
	extern pSFTP_DownloadInit				_SFTP_DownloadInit;
	extern pSFTP_DownloadPrepareData		_SFTP_DownloadPrepareData;
	extern pSFTP_DownloadData				_SFTP_DownloadData;
	extern pSFTP_IsDownloadDone				_SFTP_IsDownloadDone;
	extern pSFTP_DownloadQueue				_SFTP_DownloadQueue;
	extern pSFTP_DownloadClose				_SFTP_DownloadClose;
	extern pSFTP_DownloadCleanupPacket		_SFTP_DownloadCleanupPacket;
	extern pSFTP_DownloadCleanupTransfer	_SFTP_DownloadCleanupTransfer;
	extern pSFTP_CheckBackendStatus			_SFTP_CheckBackendStatus;
	extern pSFTP_BackendClose				_SFTP_BackendClose;
	extern pSFTP_FreeDownloadData			_SFTP_FreeDownloadData;

	extern unsigned char psftp_state;

	bool InitializePSFTP();
	bool UnInitializePSFTP();

#endif

#endif
