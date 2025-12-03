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

#ifndef _LITE_LIBCRYPTO_H
#define _LITE_LIBCRYPTO_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma push_macro( "_MSC_VER" )
#undef _MSC_VER
#define _MSC_VER 1600	// We have stdint.h, so make e_os2.h think we're an older version (VS 2010+).
#include <openssl/ssl.h>
#pragma pop_macro( "_MSC_VER" )

#include <openssl/pkcs12.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

//#define LIBCRYPTO_USE_STATIC_LIB

#ifdef LIBCRYPTO_USE_STATIC_LIB

	//__pragma( comment( lib, "libcrypto.lib" ) )

	//#define _OPENSSL_init_crypto		OPENSSL_init_crypto

	#define _BIO_new					BIO_new
	#define _BIO_s_mem					BIO_s_mem
	#define _BIO_free					BIO_free
	#define _BIO_ctrl					BIO_ctrl
	#define _BIO_read					BIO_read
	#define _BIO_write					BIO_write

	#define _BIO_new_file				BIO_new_file
	#define _d2i_PKCS12_bio				d2i_PKCS12_bio
	#define _PKCS12_parse				PKCS12_parse
	#define _PKCS12_free				PKCS12_free
	#define _EVP_PKEY_free				EVP_PKEY_free
	#define _X509_free					X509_free

#else

	#define LIBCRYPTO_STATE_SHUTDOWN	0
	#define LIBCRYPTO_STATE_RUNNING	1

	// crypto.h
	//typedef int ( WINAPIV *pOPENSSL_init_crypto )( uint64_t opts, const OPENSSL_INIT_SETTINGS *settings );

	// bio.h
	typedef BIO * ( WINAPIV *pBIO_new )( const BIO_METHOD *type );
	typedef const BIO_METHOD * ( WINAPIV *pBIO_s_mem )( void );
	typedef int ( WINAPIV *pBIO_free )( BIO *a );
	typedef long ( WINAPIV *pBIO_ctrl )( BIO *bp, int cmd, long larg, void *parg );
	typedef int ( WINAPIV *pBIO_read )( BIO *b, void *data, int dlen );
	typedef int ( WINAPIV *pBIO_write )( BIO *b, const void *data, int dlen );

	typedef BIO * ( WINAPIV *pBIO_new_file )( const char *filename, const char *mode );
	typedef PKCS12 * ( WINAPIV *pd2i_PKCS12_bio )( BIO *bp, PKCS12 **p12 );
	typedef int ( WINAPIV *pPKCS12_parse )( PKCS12 *p12, const char *pass, EVP_PKEY **pkey, X509 **cert, STACK_OF( X509 )**ca );
	typedef void ( WINAPIV *pPKCS12_free )( PKCS12 *pfx );
	typedef void ( WINAPIV *pEVP_PKEY_free )( EVP_PKEY *pkey );
	typedef void ( WINAPIV *pX509_free )( X509 *a );

	//extern pOPENSSL_init_crypto		_OPENSSL_init_crypto;

	extern pBIO_new						_BIO_new;
	extern pBIO_s_mem					_BIO_s_mem;
	extern pBIO_free					_BIO_free;
	extern pBIO_ctrl					_BIO_ctrl;
	extern pBIO_read					_BIO_read;
	extern pBIO_write					_BIO_write;

	extern pBIO_new_file				_BIO_new_file;
	extern pd2i_PKCS12_bio				_d2i_PKCS12_bio;
	extern pPKCS12_parse				_PKCS12_parse;
	extern pPKCS12_free					_PKCS12_free;
	extern pEVP_PKEY_free				_EVP_PKEY_free;
	extern pX509_free					_X509_free;

	#define _BIO_pending( b ) _BIO_ctrl( b, BIO_CTRL_PENDING, 0, NULL )

	extern unsigned char libcrypto_state;

	bool InitializeLibCrypto( wchar_t *library_path );
	bool UnInitializeLibCrypto();

#endif

#endif
