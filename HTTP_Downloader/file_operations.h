/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#define MAGIC_ID_SETTINGS		"HDM\x00"
#define MAGIC_ID_DOWNLOADS		"HDM\x12"	// Version 3

char read_config();
char save_config();

char read_download_history( wchar_t *file_path );
char save_download_history( wchar_t *file_path );

char save_download_history_csv_file( wchar_t *file_path );

#endif
