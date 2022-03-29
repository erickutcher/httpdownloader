/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2022 Eric Kutcher

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

#ifndef _SITE_MANAGER_UTILITIES_H
#define _SITE_MANAGER_UTILITIES_H

#include "globals.h"
#include "connection.h"

struct SITE_INFO
{
	PROXY_INFO			proxy_info;

	unsigned long long	download_speed_limit;

	wchar_t				*w_host;
	wchar_t				*host;
	wchar_t				*download_directory;
	wchar_t				*w_username;
	wchar_t				*w_password;

	char				*username;
	char				*password;
	char				*utf8_cookies;
	char				*utf8_headers;
	char				*utf8_data;		// POST payload.

	unsigned int		download_operations;

	PROTOCOL			protocol;
	unsigned short		port;

	unsigned char		parts;
	unsigned char		method;			// 1 = GET, 2 = POST
	char				ssl_version;	// 0 = Default, 1 = SSL 2.0, 2 = 3.0, etc.

	bool				use_download_speed_limit;
	bool				use_download_directory;
	bool				use_parts;
	bool				enable;			// Enable/Disable the site info.
};

struct SITE_UPDATE_INFO
{
	SITE_INFO *si;
	SITE_INFO *old_si;
	int index;
	unsigned char update_type;	// 0 == Add, 1 = Remove
	bool enable;
};

void FreeSiteInfo( SITE_INFO **site_info );

int dllrbt_compare_site_info( void *a, void *b );

char read_site_info();
char save_site_info();

THREAD_RETURN load_site_list( void *pArguments );
THREAD_RETURN handle_site_list( void *pArguments );

extern bool site_list_changed;
extern bool skip_site_list_draw;

#endif
