/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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

#ifndef _LITE_PCRE2_H
#define _LITE_PCRE2_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PCRE2_CODE_UNIT_WIDTH 16	// UTF-16 (wide char)
#include "pcre2.h"

//#define PCRE2_USE_STATIC_LIB

#ifdef PCRE2_USE_STATIC_LIB

	//__pragma( comment( lib, "pcre2.lib" ) )

	#define _pcre2_compile_16				pcre2_compile_16
	#define _pcre2_match_16					pcre2_match_16
	#define _pcre2_match_data_create_from_pattern_16	pcre2_match_data_create_from_pattern_16
	#define _pcre2_match_data_free_16		pcre2_match_data_free_16
	#define _pcre2_code_free_16				pcre2_code_free_16
	#define _pcre2_get_ovector_pointer_16	pcre2_get_ovector_pointer_16

#else

	#define PCRE2_STATE_SHUTDOWN	0
	#define PCRE2_STATE_RUNNING		1

	typedef pcre2_code * ( WINAPIV *ppcre2_compile_16 )( PCRE2_SPTR pattern, PCRE2_SIZE length, uint32_t options, int *errorcode, PCRE2_SIZE *erroroffset, pcre2_compile_context *ccontext ); 
	typedef int ( WINAPIV * ppcre2_match_16 )( const pcre2_code *code, PCRE2_SPTR subject, PCRE2_SIZE length, PCRE2_SIZE startoffset, uint32_t options, pcre2_match_data *match_data, pcre2_match_context *mcontext ); 
	typedef pcre2_match_data * ( WINAPIV * ppcre2_match_data_create_from_pattern_16 )( const pcre2_code *code, pcre2_general_context *gcontext ); 
	typedef void ( WINAPIV * ppcre2_match_data_free_16 )( pcre2_match_data *match_data );
	typedef void ( WINAPIV * ppcre2_code_free_16 )( pcre2_code *code );
	//typedef PCRE2_SIZE * ( WINAPIV * ppcre2_get_ovector_pointer_16 )( pcre2_match_data *match_data );

	extern ppcre2_compile_16 _pcre2_compile_16;
	extern ppcre2_match_16 _pcre2_match_16;
	extern ppcre2_match_data_create_from_pattern_16 _pcre2_match_data_create_from_pattern_16;
	extern ppcre2_match_data_free_16 _pcre2_match_data_free_16;
	extern ppcre2_code_free_16 _pcre2_code_free_16;
	//extern ppcre2_get_ovector_pointer_16 _pcre2_get_ovector_pointer_16;

	extern unsigned char pcre2_state;

	bool InitializePCRE2();
	bool UnInitializePCRE2();

#endif

#endif
