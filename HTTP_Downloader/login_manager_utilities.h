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

#ifndef _LOGIN_MANAGER_UTILITIES_H
#define _LOGIN_MANAGER_UTILITIES_H

#include "globals.h"
#include "connection.h"

struct LOGIN_INFO
{
	wchar_t				*w_username;
	wchar_t				*w_password;
	wchar_t				*w_host;
	wchar_t				*host;
	char				*username;
	char				*password;
	PROTOCOL			protocol;
	unsigned short		port;
};

struct LOGIN_UPDATE_INFO
{
	LOGIN_INFO *li;
	unsigned char update_type;	// 0 == Add, 1 = Remove
};

int dllrbt_compare_login_info( void *a, void *b );

char read_login_info();
char save_login_info();

THREAD_RETURN load_login_list( void *pArguments );
THREAD_RETURN handle_login_list( void *pArguments );

extern bool login_list_changed;
extern bool skip_login_list_draw;

#endif
