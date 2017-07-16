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

#ifndef _HTTP_PARSING_H
#define _HTTP_PARSING_H

#include "connection.h"

struct COOKIE_CONTAINER
{
	char *cookie_name;
	int name_length;
	char *cookie_value;
	int value_length;
};

unsigned short GetHTTPStatus( char *header );
void GetAuthenticate( char *header, unsigned char auth_header_type, AUTH_INFO *auth_info );
char *GetHeaderValue( char *header, char *field_name, unsigned long field_name_length, char **value_start, char **value_end );
bool ParseURL_A( char *url, PROTOCOL &protocol, char **host, unsigned short &port, char **resource );
bool ParseURL_W( wchar_t *url, PROTOCOL &protocol, wchar_t **host, unsigned short &port, wchar_t **resource );
int ParseHTTPHeader( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length/*, bool from_beginning*/ );
int GetHTTPHeader( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length );
int GetHTTPContent( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length );
bool ParseCookieValues( char *cookie_list, dllrbt_tree **cookie_tree, char **cookies );

dllrbt_tree *CopyCookieTree( dllrbt_tree *cookie_tree );

int MakeRequest( SOCKET_CONTEXT *context, IO_OPERATION next_operation, bool use_connect );
int MakeRangeRequest( SOCKET_CONTEXT *context );
int HandleRedirect( SOCKET_CONTEXT *context );


#endif
