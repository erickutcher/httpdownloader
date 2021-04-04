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

// LoadLibrary with disabled error mode
HMODULE LoadLibraryDEMW( LPCWSTR lpLibFileName )
{
	HMODULE hm = NULL;
	SetErrorMode( SEM_FAILCRITICALERRORS );	// Disable error MessageBox.
	hm = LoadLibraryW( lpLibFileName );
	SetErrorMode( 0 );						// Reset to default.

	return hm;
}

void *SetFunctionPointer( HMODULE &library, void **function_pointer, char *function_name )
{
	*function_pointer = GetProcAddress( library, function_name );
	if ( *function_pointer == NULL )
	{
		FreeLibrary( library );
		library = NULL;
	}

	return *function_pointer;
}
