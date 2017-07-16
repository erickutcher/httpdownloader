/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2017 Eric Kutcher

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

void ProcessingList( bool processing );

THREAD_RETURN remove_items( void *pArguments );
THREAD_RETURN load_download_history( void *pArguments );

THREAD_RETURN handle_download_list( void *pArguments );
THREAD_RETURN handle_connection( void *pArguments );

THREAD_RETURN handle_download_queue( void *pArguments );

#endif