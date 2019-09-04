/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2019 Eric Kutcher

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
#include "lite_crypt32.h"

#ifndef CRYPT32_USE_STATIC_LIB

	pCertFindCertificateInStore	_CertFindCertificateInStore;
	pCertCloseStore	_CertCloseStore;
	pCertFreeCertificateContext	_CertFreeCertificateContext;

	pPFXImportCertStore	_PFXImportCertStore;

	pCryptBinaryToStringA	_CryptBinaryToStringA;


	pCryptMsgClose			_CryptMsgClose;
	pCertSetCertificateContextProperty	_CertSetCertificateContextProperty;

	pCryptDecodeObjectEx	_CryptDecodeObjectEx;
	pCryptStringToBinaryA	_CryptStringToBinaryA;
	pCryptQueryObject		_CryptQueryObject;

	HMODULE hModule_crypt32 = NULL;

	unsigned char crypt32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeCrypt32()
	{
		if ( crypt32_state != CRYPT32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_crypt32 = LoadLibraryDEMW( L"crypt32.dll" );

		if ( hModule_crypt32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CertFindCertificateInStore, "CertFindCertificateInStore" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CertCloseStore, "CertCloseStore" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CertFreeCertificateContext, "CertFreeCertificateContext" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_PFXImportCertStore, "PFXImportCertStore" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CryptBinaryToStringA, "CryptBinaryToStringA" ) )


		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CryptMsgClose, "CryptMsgClose" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CertSetCertificateContextProperty, "CertSetCertificateContextProperty" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CryptDecodeObjectEx, "CryptDecodeObjectEx" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CryptStringToBinaryA, "CryptStringToBinaryA" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_crypt32, ( void ** )&_CryptQueryObject, "CryptQueryObject" ) )

		crypt32_state = CRYPT32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeCrypt32()
	{
		if ( crypt32_state != CRYPT32_STATE_SHUTDOWN )
		{
			crypt32_state = CRYPT32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_crypt32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
