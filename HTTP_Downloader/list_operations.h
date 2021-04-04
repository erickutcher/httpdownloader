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

#ifndef _LIST_OPERATIONS_H
#define _LIST_OPERATIONS_H

struct importexportinfo
{
	wchar_t *file_paths;
	unsigned int file_offset;	// Only used for importing the download history.
	unsigned char type;			// 0 = load on startup, 1 = import from menu.
};

void ProcessingList( bool processing );

THREAD_RETURN remove_items( void *pArguments );

THREAD_RETURN handle_download_list( void *pArguments );
THREAD_RETURN handle_connection( void *pArguments );

THREAD_RETURN handle_download_queue( void *pArguments );
THREAD_RETURN handle_download_update( void *pArguments );

THREAD_RETURN copy_urls( void *pArguments );

THREAD_RETURN rename_file( void *pArguments );
THREAD_RETURN delete_files( void *pArguments );

THREAD_RETURN create_download_history_csv_file( void *file_path );
THREAD_RETURN export_list( void *pArguments );
THREAD_RETURN import_list( void *pArguments );

THREAD_RETURN process_command_line_args( void *pArguments );

THREAD_RETURN search_list( void *pArguments );
THREAD_RETURN filter_urls( void *pArguments );

THREAD_RETURN save_session( void *pArguments );

#endif
