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
#include "lite_libcrypto.h"

#ifndef LIBCRYPTO_USE_STATIC_LIB

	//pOPENSSL_init_crypto		_OPENSSL_init_crypto;

	pBIO_new					_BIO_new;
	pBIO_s_mem					_BIO_s_mem;
	pBIO_free					_BIO_free;
	pBIO_ctrl					_BIO_ctrl;
	pBIO_read					_BIO_read;
	pBIO_write					_BIO_write;

	pBIO_new_file				_BIO_new_file;
	pd2i_PKCS12_bio				_d2i_PKCS12_bio;
	pPKCS12_parse				_PKCS12_parse;
	pPKCS12_free				_PKCS12_free;
	pEVP_PKEY_free				_EVP_PKEY_free;
	pX509_free					_X509_free;

	HMODULE hModule_libcrypto = NULL;

	unsigned char libcrypto_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeLibCrypto( wchar_t *library_path )
	{
		if ( libcrypto_state != LIBCRYPTO_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_libcrypto = LoadLibraryDEMW( library_path );

		if ( hModule_libcrypto == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_OPENSSL_init_crypto, "OPENSSL_init_crypto" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_new, "BIO_new" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_s_mem, "BIO_s_mem" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_free, "BIO_free" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_ctrl, "BIO_ctrl" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_read, "BIO_read" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_write, "BIO_write" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_BIO_new_file, "BIO_new_file" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_d2i_PKCS12_bio, "d2i_PKCS12_bio" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_PKCS12_parse, "PKCS12_parse" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_PKCS12_free, "PKCS12_free" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_EVP_PKEY_free, "EVP_PKEY_free" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libcrypto, ( void ** )&_X509_free, "X509_free" ) )

		libcrypto_state = LIBCRYPTO_STATE_RUNNING;

		return true;
	}

	bool UnInitializeLibCrypto()
	{
		if ( libcrypto_state != LIBCRYPTO_STATE_SHUTDOWN )
		{
			libcrypto_state = LIBCRYPTO_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_libcrypto ) == FALSE ? false : true );
		}

		return true;
	}

#endif
