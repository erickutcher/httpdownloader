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

#ifndef _LITE_CRYPT32_H
#define _LITE_CRYPT32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wincrypt.h>

//#define CRYPT32_USE_STATIC_LIB

#ifdef CRYPT32_USE_STATIC_LIB

	//__pragma( comment( lib, "crypt32.lib" ) )

	#define _CertFindCertificateInStore	CertFindCertificateInStore
	#define _CertCloseStore				CertCloseStore
	#define _CertFreeCertificateContext	CertFreeCertificateContext

	#define _PFXImportCertStore			PFXImportCertStore

	#define _CryptBinaryToStringA		CryptBinaryToStringA

	#define _CryptMsgClose				CryptMsgClose
	#define _CertSetCertificateContextProperty	CertSetCertificateContextProperty

	#define _CryptDecodeObjectEx		CryptDecodeObjectEx
	#define _CryptStringToBinaryA		CryptStringToBinaryA
	#define _CryptQueryObject			CryptQueryObject

#else

	#define CRYPT32_STATE_SHUTDOWN		0
	#define CRYPT32_STATE_RUNNING		1

	typedef PCCERT_CONTEXT ( WINAPI *pCertFindCertificateInStore )( HCERTSTORE hCertStore, DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType, const void *pvFindPara, PCCERT_CONTEXT pPrevCertContext );
	typedef BOOL ( WINAPI *pCertCloseStore )( HCERTSTORE hCertStore, DWORD dwFlags );
	typedef BOOL ( WINAPI *pCertFreeCertificateContext )( PCCERT_CONTEXT pCertContext );

	typedef HCERTSTORE ( WINAPI *pPFXImportCertStore )( CRYPT_DATA_BLOB *pPFX, LPCWSTR szPassword, DWORD dwFlags );

	typedef BOOL ( WINAPI *pCryptBinaryToStringA )( const BYTE *pbBinary, DWORD cbBinary, DWORD dwFlags, LPSTR pszString, DWORD *pcchString );

	typedef BOOL ( WINAPI *pCryptMsgClose )( HCRYPTMSG hCryptMsg );
	typedef BOOL ( WINAPI *pCertSetCertificateContextProperty )( PCCERT_CONTEXT pCertContext, DWORD dwPropId, DWORD dwFlags, const void *pvData );

	typedef BOOL ( WINAPI *pCryptDecodeObjectEx )( DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo );
	typedef BOOL ( WINAPI *pCryptStringToBinaryA )( LPCSTR pszString, DWORD cchString, DWORD dwFlags, BYTE *pbBinary, DWORD *pcbBinary, DWORD *pdwSkip, DWORD *pdwFlags );
	typedef BOOL ( WINAPI *pCryptQueryObject )( DWORD dwObjectType, const void *pvObject, DWORD dwExpectedContentTypeFlags, DWORD dwExpectedFormatTypeFlags, DWORD dwFlags, DWORD *pdwMsgAndCertEncodingType, DWORD *pdwContentType, DWORD *pdwFormatType, HCERTSTORE *phCertStore, HCRYPTMSG *phMsg, const void **ppvContext );

	extern pCertFindCertificateInStore	_CertFindCertificateInStore;
	extern pCertCloseStore	_CertCloseStore;
	extern pCertFreeCertificateContext	_CertFreeCertificateContext;

	extern pPFXImportCertStore	_PFXImportCertStore;

	extern pCryptBinaryToStringA	_CryptBinaryToStringA;

	extern pCryptMsgClose		_CryptMsgClose;
	extern pCertSetCertificateContextProperty	_CertSetCertificateContextProperty;

	extern pCryptDecodeObjectEx	_CryptDecodeObjectEx;
	extern pCryptStringToBinaryA	_CryptStringToBinaryA;
	extern pCryptQueryObject	_CryptQueryObject;

	extern unsigned char crypt32_state;

	bool InitializeCrypt32();
	bool UnInitializeCrypt32();

#endif

#endif
