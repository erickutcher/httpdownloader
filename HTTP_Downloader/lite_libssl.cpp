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
#include "lite_libssl.h"

#ifndef LIBSSL_USE_STATIC_LIB

	//pOPENSSL_init_ssl			_OPENSSL_init_ssl;
	pSSL_CTX_new				_SSL_CTX_new;
	pSSL_CTX_ctrl				_SSL_CTX_ctrl;
	pSSL_CTX_set_min_proto_version	_SSL_CTX_set_min_proto_version;
	pSSL_CTX_set_max_proto_version	_SSL_CTX_set_max_proto_version;
	pSSL_CTX_set_grease_enabled	_SSL_CTX_set_grease_enabled;
	pSSL_CTX_set_session_cache_mode	_SSL_CTX_set_session_cache_mode;
	//pSSL_CTX_set_max_early_data	_SSL_CTX_set_max_early_data;
	pSSL_CTX_sess_set_new_cb	_SSL_CTX_sess_set_new_cb;
	pSSL_CTX_free				_SSL_CTX_free;
	pSSL_new					_SSL_new;
	pSSL_get_ex_data			_SSL_get_ex_data;
	pSSL_set_ex_data			_SSL_set_ex_data;
	pSSL_connect				_SSL_connect;
	pSSL_accept					_SSL_accept;
	pSSL_read					_SSL_read;
	pSSL_write					_SSL_write;
	pSSL_free					_SSL_free;
	pTLS_client_method			_TLS_client_method;
	pTLS_server_method			_TLS_server_method;
	pSSL_set_fd					_SSL_set_fd;
	pSSL_pending				_SSL_pending;
	pSSL_ctrl					_SSL_ctrl;
	pSSL_set_tlsext_host_name	_SSL_set_tlsext_host_name;
	pSSL_set_bio				_SSL_set_bio;
	pSSL_shutdown				_SSL_shutdown;
	pSSL_get_error				_SSL_get_error;
	pSSL_CTX_use_certificate_file	_SSL_CTX_use_certificate_file;
	pSSL_CTX_use_PrivateKey_file	_SSL_CTX_use_PrivateKey_file;
	pSSL_CTX_use_certificate	_SSL_CTX_use_certificate;
	pSSL_CTX_use_PrivateKey		_SSL_CTX_use_PrivateKey;
	//pSSL_get1_session			_SSL_get1_session;
	pSSL_set_session			_SSL_set_session;
	pSSL_SESSION_up_ref			_SSL_SESSION_up_ref;
	pSSL_SESSION_free			_SSL_SESSION_free;
	//pSSL_session_reused			_SSL_session_reused;
	//pSSL_SESSION_is_resumable	_SSL_SESSION_is_resumable;
	//pSSL_SESSION_dup			_SSL_SESSION_dup;

	HMODULE hModule_libssl = NULL;

	unsigned char libssl_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeLibSSL( wchar_t *library_path )
	{
		if ( libssl_state != LIBSSL_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_libssl = LoadLibraryDEMW( library_path );

		if ( hModule_libssl == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_OPENSSL_init_ssl, "OPENSSL_init_ssl" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_new, "SSL_CTX_new" ) )

		_SSL_CTX_ctrl = ( pSSL_CTX_ctrl )GetProcAddress( hModule_libssl, "SSL_CTX_ctrl" );
		// These are functions in BoringSSL, but macros for SSL_CTX_ctrl in OpenSSL.
		_SSL_CTX_set_min_proto_version = ( pSSL_CTX_set_min_proto_version )GetProcAddress( hModule_libssl, "SSL_CTX_set_min_proto_version" );
		_SSL_CTX_set_max_proto_version = ( pSSL_CTX_set_max_proto_version )GetProcAddress( hModule_libssl, "SSL_CTX_set_max_proto_version" );
		_SSL_CTX_set_session_cache_mode = ( pSSL_CTX_set_session_cache_mode )GetProcAddress( hModule_libssl, "SSL_CTX_set_session_cache_mode" );

		if ( _SSL_CTX_ctrl == NULL && ( _SSL_CTX_set_max_proto_version == NULL ||
										_SSL_CTX_set_max_proto_version == NULL ||
										_SSL_CTX_set_session_cache_mode == NULL ) )
		{
			return false;
		}

		// This is a function in BoringSSL. It's fine if it doesn't load.
		_SSL_CTX_set_grease_enabled = ( pSSL_CTX_set_grease_enabled )GetProcAddress( hModule_libssl, "SSL_CTX_set_grease_enabled" );

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_set_max_early_data, "SSL_CTX_set_max_early_data" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_sess_set_new_cb, "SSL_CTX_sess_set_new_cb" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_free, "SSL_CTX_free" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_new, "SSL_new" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_get_ex_data, "SSL_get_ex_data" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_set_ex_data, "SSL_set_ex_data" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_connect, "SSL_connect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_accept, "SSL_accept" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_read, "SSL_read" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_write, "SSL_write" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_free, "SSL_free" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_TLS_client_method, "TLS_client_method" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_TLS_server_method, "TLS_server_method" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_set_fd, "SSL_set_fd" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_pending, "SSL_pending" ) )

		_SSL_ctrl = ( pSSL_ctrl )GetProcAddress( hModule_libssl, "SSL_ctrl" );
		// This is a function in BoringSSL, but a macro for SSL_ctrl in OpenSSL.
		_SSL_set_tlsext_host_name = ( pSSL_set_tlsext_host_name )GetProcAddress( hModule_libssl, "SSL_set_tlsext_host_name" );

		if ( _SSL_ctrl == NULL && _SSL_set_tlsext_host_name == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_set_bio, "SSL_set_bio" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_shutdown, "SSL_shutdown" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_get_error, "SSL_get_error" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_use_certificate_file, "SSL_CTX_use_certificate_file" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_use_PrivateKey_file, "SSL_CTX_use_PrivateKey_file" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_use_certificate, "SSL_CTX_use_certificate" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_CTX_use_PrivateKey, "SSL_CTX_use_PrivateKey" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_get1_session, "SSL_get1_session" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_set_session, "SSL_set_session" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_SESSION_up_ref, "SSL_SESSION_up_ref" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_SESSION_free, "SSL_SESSION_free" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_session_reused, "SSL_session_reused" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_SESSION_is_resumable, "SSL_SESSION_is_resumable" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_libssl, ( void ** )&_SSL_SESSION_dup, "SSL_SESSION_dup" ) )

		libssl_state = LIBSSL_STATE_RUNNING;

		return true;
	}

	bool UnInitializeLibSSL()
	{
		if ( libssl_state != LIBSSL_STATE_SHUTDOWN )
		{
			libssl_state = LIBSSL_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_libssl ) == FALSE ? false : true );
		}

		return true;
	}

#endif
