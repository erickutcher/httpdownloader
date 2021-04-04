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

#ifndef _FILE_OPERATIONS_H
#define _FILE_OPERATIONS_H

#define MAGIC_ID_SETTINGS			"HDM\x07"	// Version 8
#define MAGIC_ID_DOWNLOADS			"HDM\x15"	// Version 6
#define MAGIC_ID_SITES				"HDM\x21"	// Version 2
#define MAGIC_ID_SFTP_HOSTS			"HDM\x30"	// Version 1
#define MAGIC_ID_SFTP_KEYS			"HDM\x40"	// Version 1

char read_config();
char save_config();

char read_download_history( wchar_t *file_path );
char save_download_history( wchar_t *file_path );

char save_download_history_csv_file( wchar_t *file_path );

wchar_t *read_url_list_file( wchar_t *file_path, unsigned int &url_list_length );

wchar_t *UTF8StringToWideString( char *utf8_string, int string_length );
char *WideStringToUTF8String( wchar_t *wide_string, int *utf8_string_length, int buffer_offset = 0 );

#endif
