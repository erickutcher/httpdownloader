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

#ifndef _LITE_LIBSSL_H
#define _LITE_LIBSSL_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma push_macro( "_MSC_VER" )
#undef _MSC_VER
#define _MSC_VER 1600	// We have stdint.h, so make e_os2.h think we're an older version (VS 2010+).
#include <openssl/ssl.h>
#include <openssl/ssl2.h>
#pragma pop_macro( "_MSC_VER" )

//#define LIBSSL_USE_STATIC_LIB

#ifdef LIBSSL_USE_STATIC_LIB

	//__pragma( comment( lib, "libssl.lib" ) )
	//__pragma( comment( lib, "libcrypto.lib" ) )

	//#define _OPENSSL_init_ssl			OPENSSL_init_ssl
	#define _SSL_CTX_new				SSL_CTX_new
	#define _SSL_CTX_ctrl				SSL_CTX_ctrl
	#define _SSL_CTX_set_min_proto_version	SSL_CTX_set_min_proto_version
	#define _SSL_CTX_set_max_proto_version	SSL_CTX_set_max_proto_version
	#define _SSL_CTX_set_grease_enabled	SSL_CTX_set_grease_enabled
	#define _SSL_CTX_set_session_cache_mode	SSL_CTX_set_session_cache_mode
	//#define _SSL_CTX_set_max_early_data	SSL_CTX_set_max_early_data
	#define _SSL_CTX_sess_set_new_cb	SSL_CTX_sess_set_new_cb
	#define _SSL_CTX_free				SSL_CTX_free
	#define _SSL_new					SSL_new
	#define _SSL_get_ex_data			SSL_get_ex_data
	#define _SSL_set_ex_data			SSL_set_ex_data
	#define _SSL_connect				SSL_connect
	#define _SSL_accept					SSL_accept
	#define _SSL_read					SSL_read
	#define _SSL_write					SSL_write
	#define _SSL_free					SSL_free
	#define _TLS_client_method			TLS_client_method
	#define _TLS_server_method			TLS_server_method
	#define _SSL_set_fd					SSL_set_fd
	#define _SSL_pending				SSL_pending
	#define _SSL_ctrl					SSL_ctrl
	#define _SSL_set_tlsext_host_name	SSL_set_tlsext_host_name
	#define _SSL_set_bio				SSL_set_bio
	#define _SSL_shutdown				SSL_shutdown
	#define _SSL_get_error				SSL_get_error
	#define _SSL_CTX_use_certificate_file	SSL_CTX_use_certificate_file
	#define _SSL_CTX_use_PrivateKey_file	SSL_CTX_use_PrivateKey_file
	#define _SSL_CTX_use_certificate	SSL_CTX_use_certificate
	#define _SSL_CTX_use_PrivateKey		SSL_CTX_use_PrivateKey
	//#define _SSL_get1_session			SSL_get1_session
	#define _SSL_set_session			SSL_set_session
	#define _SSL_SESSION_up_ref			SSL_SESSION_up_ref
	#define _SSL_SESSION_free			SSL_SESSION_free
	//#define _SSL_session_reused			SSL_session_reused
	//#define _SSL_SESSION_is_resumable	SSL_SESSION_is_resumable
	//#define _SSL_SESSION_dup			SSL_SESSION_dup

#else

	#define LIBSSL_STATE_SHUTDOWN	0
	#define LIBSSL_STATE_RUNNING	1

	//typedef int ( WINAPIV *pOPENSSL_init_ssl )( uint64_t opts, const OPENSSL_INIT_SETTINGS *settings );
	typedef SSL_CTX * ( WINAPIV *pSSL_CTX_new )( const SSL_METHOD *meth );
	typedef long ( WINAPIV *pSSL_CTX_ctrl )( SSL_CTX *ctx, int cmd, long larg, void *parg );
	typedef int ( WINAPIV *pSSL_CTX_set_min_proto_version )( SSL_CTX *ctx, uint16_t version );	// BoringSSL
	typedef int ( WINAPIV *pSSL_CTX_set_max_proto_version )( SSL_CTX *ctx, uint16_t version );	// BoringSSL
	typedef void ( WINAPIV *pSSL_CTX_set_grease_enabled )( SSL_CTX *ctx, int enabled );			// BoringSSL
	typedef int ( WINAPIV *pSSL_CTX_set_session_cache_mode )( SSL_CTX *ctx, int mode );			// BoringSSL
	//typedef int ( WINAPIV *pSSL_CTX_set_max_early_data )( SSL_CTX *ctx, uint32_t max_early_data );
	typedef void ( WINAPIV *pSSL_CTX_sess_set_new_cb )( SSL_CTX *ctx, int( * new_session_cb )( struct ssl_st *ssl, SSL_SESSION *sess ) );
	typedef void ( WINAPIV *pSSL_CTX_free )( SSL_CTX *ctx );
	typedef SSL * ( WINAPIV *pSSL_new )( SSL_CTX *ctx );
	typedef void * ( WINAPIV *pSSL_get_ex_data )( const SSL *ssl, int idx );
	typedef int ( WINAPIV *pSSL_set_ex_data )( SSL *ssl, int idx, void *data );
	typedef int ( WINAPIV *pSSL_connect )( SSL *ssl );
	typedef int ( WINAPIV *pSSL_accept )( SSL *ssl );
	typedef int ( WINAPIV *pSSL_read )( SSL *ssl, void *buf, int num );
	typedef int ( WINAPIV *pSSL_write )( SSL *ssl, const void *buf, int num );
	typedef void ( WINAPIV *pSSL_free )( SSL *ssl );
	typedef const SSL_METHOD * ( WINAPIV *pTLS_client_method )( void );
	typedef const SSL_METHOD * ( WINAPIV *pTLS_server_method )( void );
	typedef int ( WINAPIV *pSSL_set_fd )( SSL *ssl, int fd );
	typedef int ( WINAPIV *pSSL_pending )( const SSL *s );
	typedef long ( WINAPIV *pSSL_ctrl )( SSL *ssl, int cmd, long larg, void *parg );
	typedef int ( WINAPIV *pSSL_set_tlsext_host_name )( SSL *ssl, const char *hostname );		// BoringSSL
	typedef void ( WINAPIV *pSSL_set_bio )( SSL *s, BIO *rbio, BIO *wbio );
	typedef int ( WINAPIV *pSSL_shutdown )( SSL *s );
	typedef int ( WINAPIV *pSSL_get_error )( const SSL *s, int ret_code );
	typedef int ( WINAPIV *pSSL_CTX_use_certificate_file )( SSL_CTX *ctx, const char *file, int type );
	typedef int ( WINAPIV *pSSL_CTX_use_PrivateKey_file )( SSL_CTX *ctx, const char *file, int type );
	typedef int ( WINAPIV *pSSL_CTX_use_certificate )( SSL_CTX *ctx, X509 *x );
	typedef int ( WINAPIV *pSSL_CTX_use_PrivateKey )( SSL_CTX *ctx, EVP_PKEY *pkey );
	//typedef SSL_SESSION * ( WINAPIV *pSSL_get1_session )( SSL *ssl );
	typedef int ( WINAPIV *pSSL_set_session )( SSL *to, SSL_SESSION *session );
	typedef int ( WINAPIV *pSSL_SESSION_up_ref )( SSL_SESSION *ses );
	typedef void ( WINAPIV *pSSL_SESSION_free )( SSL_SESSION *ses );
	//typedef int ( WINAPIV *pSSL_session_reused )( const SSL *s );
	//typedef int ( WINAPIV *pSSL_SESSION_is_resumable )( const SSL_SESSION *s );
	//typedef SSL_SESSION * ( WINAPIV *pSSL_SESSION_dup )( const SSL_SESSION *src );

	//extern pOPENSSL_init_ssl			_OPENSSL_init_ssl;
	extern pSSL_CTX_new					_SSL_CTX_new;
	extern pSSL_CTX_ctrl				_SSL_CTX_ctrl;
	extern pSSL_CTX_set_min_proto_version	_SSL_CTX_set_min_proto_version;
	extern pSSL_CTX_set_max_proto_version	_SSL_CTX_set_max_proto_version;
	extern pSSL_CTX_set_grease_enabled	_SSL_CTX_set_grease_enabled;
	extern pSSL_CTX_set_session_cache_mode	_SSL_CTX_set_session_cache_mode;
	//extern pSSL_CTX_set_max_early_data	_SSL_CTX_set_max_early_data;
	extern pSSL_CTX_sess_set_new_cb		_SSL_CTX_sess_set_new_cb;
	extern pSSL_CTX_free				_SSL_CTX_free;
	extern pSSL_new						_SSL_new;
	extern pSSL_get_ex_data				_SSL_get_ex_data;
	extern pSSL_set_ex_data				_SSL_set_ex_data;
	extern pSSL_connect					_SSL_connect;
	extern pSSL_accept					_SSL_accept;
	extern pSSL_read					_SSL_read;
	extern pSSL_write					_SSL_write;
	extern pSSL_free					_SSL_free;
	extern pTLS_client_method			_TLS_client_method;
	extern pTLS_server_method			_TLS_server_method;
	extern pSSL_set_fd					_SSL_set_fd;
	extern pSSL_pending					_SSL_pending;
	extern pSSL_ctrl					_SSL_ctrl;
	extern pSSL_set_tlsext_host_name	_SSL_set_tlsext_host_name;
	extern pSSL_set_bio					_SSL_set_bio;
	extern pSSL_shutdown				_SSL_shutdown;
	extern pSSL_get_error				_SSL_get_error;
	extern pSSL_CTX_use_certificate_file	_SSL_CTX_use_certificate_file;
	extern pSSL_CTX_use_PrivateKey_file	_SSL_CTX_use_PrivateKey_file;
	extern pSSL_CTX_use_certificate		_SSL_CTX_use_certificate;
	extern pSSL_CTX_use_PrivateKey		_SSL_CTX_use_PrivateKey;
	//extern pSSL_get1_session			_SSL_get1_session;
	extern pSSL_set_session				_SSL_set_session;
	extern pSSL_SESSION_up_ref			_SSL_SESSION_up_ref;
	extern pSSL_SESSION_free			_SSL_SESSION_free;
	//extern pSSL_session_reused			_SSL_session_reused;
	//extern pSSL_SESSION_is_resumable	_SSL_SESSION_is_resumable;
	//extern pSSL_SESSION_dup				_SSL_SESSION_dup;

	extern unsigned char libssl_state;

	bool InitializeLibSSL( wchar_t *library_path );
	bool UnInitializeLibSSL();

	//#define _SSL_get_error_s( s, ret_code )	( ( s ) != NULL ? _SSL_get_error( ( s ), ( ret_code ) ) : 1 )

#endif

#endif
