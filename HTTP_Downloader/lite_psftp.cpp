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

#include "lite_dlls.h"
#include "lite_psftp.h"

#ifndef PSFTP_USE_STATIC_LIB

	pSFTP_InitGSSAPI				_SFTP_InitGSSAPI;
	pSFTP_UninitGSSAPI				_SFTP_UninitGSSAPI;
	pSFTP_SetConfigInfo				_SFTP_SetConfigInfo;
	pSFTP_SetAlgorithmPriorities	_SFTP_SetAlgorithmPriorities;
	pSFTP_CreateSSHHandle			_SFTP_CreateSSHHandle;
	pSFTP_BackendFree				_SFTP_BackendFree;
	pSFTP_FreeSSHHandle				_SFTP_FreeSSHHandle;
	pSFTP_CheckCallbacks			_SFTP_CheckCallbacks;
	pSFTP_RunCallbacks				_SFTP_RunCallbacks;
	pSFTP_GetKeyInfo				_SFTP_GetKeyInfo;
	pSFTP_ProcessWriteRequest		_SFTP_ProcessWriteRequest;
	pSFTP_CheckInitStatus			_SFTP_CheckInitStatus;
	pSFTP_InitSendVersion			_SFTP_InitSendVersion;
	pSFTP_ProcessGetRequestBuffer	_SFTP_ProcessGetRequestBuffer;
	pSFTP_GetStatus					_SFTP_GetStatus;
	pSFTP_SetStatus					_SFTP_SetStatus;
	pSFTP_CheckCallbackStatus		_SFTP_CheckCallbackStatus;
	pSFTP_GetRequestPacket			_SFTP_GetRequestPacket;
	pSFTP_GetPacketInfo				_SFTP_GetPacketInfo;
	pSFTP_ResetPacketInfo			_SFTP_ResetPacketInfo;
	pSFTP_PrepareRequestPacket		_SFTP_PrepareRequestPacket;
	pSFTP_GetRequestPacketType		_SFTP_GetRequestPacketType;
	pSFTP_ProcessVersion			_SFTP_ProcessVersion;
	pSFTP_GetAttributes				_SFTP_GetAttributes;
	pSFTP_ProcessAttributes			_SFTP_ProcessAttributes;
	pSFTP_GetHandle					_SFTP_GetHandle;
	pSFTP_ProcessDownloadHandle		_SFTP_ProcessDownloadHandle;
	pSFTP_DownloadInit				_SFTP_DownloadInit;
	pSFTP_DownloadPrepareData		_SFTP_DownloadPrepareData;
	pSFTP_DownloadData				_SFTP_DownloadData;
	pSFTP_IsDownloadDone			_SFTP_IsDownloadDone;
	pSFTP_DownloadQueue				_SFTP_DownloadQueue;
	pSFTP_DownloadClose				_SFTP_DownloadClose;
	pSFTP_DownloadCleanupPacket		_SFTP_DownloadCleanupPacket;
	pSFTP_DownloadCleanupTransfer	_SFTP_DownloadCleanupTransfer;
	pSFTP_CheckBackendStatus		_SFTP_CheckBackendStatus;
	pSFTP_BackendClose				_SFTP_BackendClose;
	pSFTP_FreeDownloadData			_SFTP_FreeDownloadData;

	HMODULE hModule_psftp = NULL;

	unsigned char psftp_state = 0;	// 0 = Not running, 1 = running.

	bool InitializePSFTP()
	{
		if ( psftp_state != PSFTP_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_psftp = LoadLibraryDEMW( L"psftp.dll" );

		if ( hModule_psftp == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_InitGSSAPI, "SFTP_InitGSSAPI" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_UninitGSSAPI, "SFTP_UninitGSSAPI" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_SetConfigInfo, "SFTP_SetConfigInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_SetAlgorithmPriorities, "SFTP_SetAlgorithmPriorities" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_CreateSSHHandle, "SFTP_CreateSSHHandle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_BackendFree, "SFTP_BackendFree" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_FreeSSHHandle, "SFTP_FreeSSHHandle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_CheckCallbacks, "SFTP_CheckCallbacks" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_RunCallbacks, "SFTP_RunCallbacks" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetKeyInfo, "SFTP_GetKeyInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ProcessWriteRequest, "SFTP_ProcessWriteRequest" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_CheckInitStatus, "SFTP_CheckInitStatus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_InitSendVersion, "SFTP_InitSendVersion" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ProcessGetRequestBuffer, "SFTP_ProcessGetRequestBuffer" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetStatus, "SFTP_GetStatus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_SetStatus, "SFTP_SetStatus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_CheckCallbackStatus, "SFTP_CheckCallbackStatus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetRequestPacket, "SFTP_GetRequestPacket" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetPacketInfo, "SFTP_GetPacketInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ResetPacketInfo, "SFTP_ResetPacketInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_PrepareRequestPacket, "SFTP_PrepareRequestPacket" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetRequestPacketType, "SFTP_GetRequestPacketType" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ProcessVersion, "SFTP_ProcessVersion" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetAttributes, "SFTP_GetAttributes" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ProcessAttributes, "SFTP_ProcessAttributes" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_GetHandle, "SFTP_GetHandle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_ProcessDownloadHandle, "SFTP_ProcessDownloadHandle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadInit, "SFTP_DownloadInit" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadPrepareData, "SFTP_DownloadPrepareData" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadData, "SFTP_DownloadData" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_IsDownloadDone, "SFTP_IsDownloadDone" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadQueue, "SFTP_DownloadQueue" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadClose, "SFTP_DownloadClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadCleanupPacket, "SFTP_DownloadCleanupPacket" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_DownloadCleanupTransfer, "SFTP_DownloadCleanupTransfer" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_CheckBackendStatus, "SFTP_CheckBackendStatus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_BackendClose, "SFTP_BackendClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_psftp, ( void ** )&_SFTP_FreeDownloadData, "SFTP_FreeDownloadData" ) )

		psftp_state = PSFTP_STATE_RUNNING;

		return true;
	}

	bool UnInitializePSFTP()
	{
		if ( psftp_state != PSFTP_STATE_SHUTDOWN )
		{
			psftp_state = PSFTP_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_psftp ) == FALSE ? false : true );
		}

		return true;
	}

#endif
