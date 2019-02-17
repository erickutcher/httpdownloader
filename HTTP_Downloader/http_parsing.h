/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
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

char *GetHeaderValue( char *header, char *field_name, unsigned long field_name_length, char **value_start, char **value_end );
bool ParseURL_A( char *url, char *original_resource, PROTOCOL &protocol, char **host, unsigned int &host_length, unsigned short &port, char **resource, unsigned int &resource_length );
bool ParseURL_W( wchar_t *url, wchar_t *original_resource, PROTOCOL &protocol, wchar_t **host, unsigned int &host_length, unsigned short &port, wchar_t **resource, unsigned int &resource_length );
bool ParseCookieValues( char *cookie_list, dllrbt_tree **cookie_tree, char **cookies );

unsigned short GetHTTPStatus( char *header );
void GetAuthorization( char *header, AUTH_INFO *auth_info );
void GetAuthenticate( char *header, unsigned char auth_header_type, AUTH_INFO *auth_info );
bool GetTransferEncoding( char *header );
unsigned long long GetContentLength( char *header );
void GetContentRange( char *header, RANGE_INFO *range_info );
void GetLocation( char *header, URL_LOCATION *url_location );
unsigned char GetConnection( char *header );
unsigned char GetContentEncoding( char *header );
char *GetContentDisposition( char *header, unsigned int &filename_length );
//char *GetETag( char *header );

dllrbt_tree *CopyCookieTree( dllrbt_tree *cookie_tree );

char ParseHTTPHeader( SOCKET_CONTEXT *context, char *header_buffer, unsigned int header_buffer_length, bool request = false );
char GetHTTPHeader( SOCKET_CONTEXT *context, char *header_buffer, unsigned int header_buffer_length );
char GetHTTPResponseContent( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length );
char GetHTTPRequestContent( SOCKET_CONTEXT *context, char *request_buffer, unsigned int request_buffer_length );

char MakeResponse( SOCKET_CONTEXT *context );
char MakeRequest( SOCKET_CONTEXT *context, IO_OPERATION next_operation, bool use_connect );
char MakeRangeRequest( SOCKET_CONTEXT *context );
char HandleRedirect( SOCKET_CONTEXT *context );

#endif
