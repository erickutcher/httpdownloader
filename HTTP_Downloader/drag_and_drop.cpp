/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2025 Eric Kutcher

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

#include "globals.h"
#include "utilities.h"
#include "lite_ntdll.h"
#include "lite_ole32.h"

#include "drag_and_drop.h"
#include "list_operations.h"
#include "categories.h"

void HandleFileList( HDROP hdrop )
{
	UINT count = _DragQueryFileW( hdrop, 0xFFFFFFFF, NULL, 0 );

	importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
	if ( iei != NULL )
	{
		iei->file_paths = NULL;
		iei->file_offset = 0;
		iei->type = 1;	// Import from menu.

		wchar_t file_path[ MAX_PATH ];

		int file_paths_offset = 0;	// Keeps track of the last file in filepath.
		int file_paths_length = ( MAX_PATH * count ) + 1;

		// Go through the list of paths.
		for ( UINT i = 0; i < count; ++i )
		{
			// Get the file path and its length.
			int file_path_length = _DragQueryFileW( hdrop, i, file_path, MAX_PATH ) + 1;	// Include the NULL terminator.

			// Skip any folders that were dropped.
			DWORD gfa = GetFileAttributesW( file_path );
			if ( gfa != INVALID_FILE_ATTRIBUTES && !( gfa & FILE_ATTRIBUTE_DIRECTORY ) )
			{
				if ( iei->file_paths == NULL )
				{
					iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * file_paths_length );
					if ( iei->file_paths != NULL )
					{
						iei->file_offset = file_path_length;

						// Find the last occurance of "\" in the string.
						while ( iei->file_offset != 0 && file_path[ --iei->file_offset ] != L'\\' );

						// Save the root directory name.
						_wmemcpy_s( iei->file_paths, file_paths_length - iei->file_offset, file_path, iei->file_offset );

						file_paths_offset = ++iei->file_offset;
					}
				}

				if ( iei->file_paths != NULL )
				{
					// Copy the file name. Each is separated by the NULL character.
					_wmemcpy_s( iei->file_paths + file_paths_offset, file_paths_length - file_paths_offset, file_path + iei->file_offset, file_path_length - iei->file_offset );

					file_paths_offset += ( file_path_length - iei->file_offset );
				}
			}
		}

		// Can't do this because ReleaseStgMedium will release the memory again (double free). Should only be called in WM_DROPFILES.
		//_DragFinish( hdrop );

		if ( iei->file_paths != NULL )
		{
			// iei will be freed in the import_list thread.
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
			else
			{
				GlobalFree( iei->file_paths );
				GlobalFree( iei );
			}
		}
		else	// No files were dropped.
		{
			GlobalFree( iei );
		}
	}
}

void HandleAddInfo( UINT cfFormat, PVOID data )
{
	ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );
	if ( ai != NULL )
	{
		ai->method = METHOD_GET;

		if ( cfg_drag_and_drop_action == DRAG_AND_DROP_ACTION_ADD_IN_STOPPED_STATE )
		{
			ai->download_operations = DOWNLOAD_OPERATION_ADD_STOPPED;
		}
		else if ( cfg_drag_and_drop_action == DRAG_AND_DROP_ACTION_VERIFY )
		{
			ai->download_operations = DOWNLOAD_OPERATION_VERIFY;
		}
		else
		{
			ai->download_operations = DOWNLOAD_OPERATION_NONE;
		}

		//ai->download_operations = DOWNLOAD_OPERATION_SIMULATE;	// For testing.

		if ( cfFormat == CF_UNICODETEXT || cfFormat == CF_HTML )
		{
			ai->urls = GlobalStrDupW( ( wchar_t * )data );
		}
		else// if ( cfFormat == CF_TEXT )
		{
			int urls_length = MultiByteToWideChar( CP_UTF8, 0, ( char * )data, -1, NULL, 0 );	// Include the NULL terminator.
			ai->urls = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * urls_length );
			MultiByteToWideChar( CP_UTF8, 0, ( char * )data, -1, ai->urls, urls_length );
		}

		// ai is freed in AddURL.
		HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL );
		if ( thread != NULL )
		{
			CloseHandle( thread );
		}
		else
		{
			GlobalFree( ai->urls );
			GlobalFree( ai );
		}
	}
}

// Finds a character in an element, but excludes searching attribute values.
char *FindCharExcludeAttributeValues( char *element_start, char *element_end, char element_character )
{
	unsigned char opened_quote = 0;

	char *pos = element_start;

	while ( pos != NULL )
	{
		// If an end was supplied, then search until we reach it. If not, then search until we reach the NULL terminator.
		if ( ( element_end != NULL && pos < element_end ) || ( element_end == NULL && *pos != NULL ) )
		{
			if ( opened_quote == 0 )
			{
				// Exit if we've found the end of the element.
				if ( *pos == element_character )
				{
					break;
				}
				else if ( *pos == '\'' || *pos == '\"' )	// A single or double quote has been opened.
				{
					opened_quote = *pos;
				}
			}
			else	// Find the single or double quote's pair (closing quote).
			{
				if ( *pos == opened_quote )
				{
					opened_quote = 0;
				}
			}

			++pos;
		}
		else
		{
			pos = NULL;
		}
	}

	return pos;
}

wchar_t *ParseHTMLClipboard( char *data )
{
	wchar_t *w_url_buffer = NULL;
	char *url_buffer = NULL;
	unsigned int url_buffer_length = 0;		// Length of data in the url buffer.
	unsigned int url_buffer_size = 8192;	// The size of the buffer.

	char *element_start = data;
	char *element_end = data;
	char *attribute_name_start = NULL;
	char *attribute_name_end = NULL;
	char *attribute_value_start = NULL;
	char *attribute_value_end = NULL;
	char *href = NULL;
	char *href_end = NULL;

	char *source_url_end = NULL;		// The last directory of the URL.
	char *source_url_root_end = NULL;	// The root directory of the URL (the end of the domain name).

	char *source_url_start = _StrStrA( data, "SourceURL:" );
	if ( source_url_start != NULL )
	{
		source_url_start += 10;

		char found_domain = 0;
		char *last_directory = NULL;

		source_url_end = source_url_start;

		while ( *source_url_end != NULL )
		{
			// Save the last and root directory offsets.
			if ( *source_url_end == '/' )
			{
				if ( source_url_end > source_url_start )
				{
					// If the previous value and the current form "//", then we'll assume that it's the start of the domain.
					if ( *( source_url_end - 1 ) == '/' )
					{
						if ( found_domain == 0 )
						{
							// Compare the previous characters in the string to see if it's an HTTP/S / FTP/S/ES / SFTP URL.
							// We already know that the last two characters are "//".
							if ( ( ( source_url_end - source_url_start ) == 6 && _StrCmpNIA( source_url_start, "http:", 5 ) == 0 ) ||
								 ( ( source_url_end - source_url_start ) == 7 && _StrCmpNIA( source_url_start, "https:", 6 ) == 0 ) ||
								 ( ( source_url_end - source_url_start ) == 5 && _StrCmpNIA( source_url_start, "ftp:", 4 ) == 0 ) ||
								 ( ( source_url_end - source_url_start ) == 6 && _StrCmpNIA( source_url_start, "ftps:", 5 ) == 0 ) ||
								 ( ( source_url_end - source_url_start ) == 7 && _StrCmpNIA( source_url_start, "ftpes:", 6 ) == 0 ) ||
								 ( ( source_url_end - source_url_start ) == 6 && _StrCmpNIA( source_url_start, "sftp:", 5 ) == 0 ) )
							{
								found_domain = 1;
							}
							else	// Not an HTTP/S / FTP/S/ES / SFTP URL.
							{
								source_url_start = source_url_end = source_url_root_end = NULL;

								break;
							}
						}
					}
					else
					{
						last_directory = source_url_end;	// The last directory.

						// We found the start of the domain, so this will be the end of the domain.
						if ( found_domain == 1 )
						{
							source_url_root_end = source_url_end;

							found_domain = 2;	// Don't try to find the domain anymore.
						}
					}
				}
			}

			if ( *source_url_end == '\r' || *source_url_end == '\n' )
			{
				// We'll set the end to the last directory so that we can save relative urls.
				if ( last_directory != NULL )
				{
					source_url_end = last_directory;
				}

				break;
			}

			++source_url_end;
		}
	}

	for ( ;; )
	{
		// Look for the opening of an HTML element.
		element_start = _StrChrA( element_end, '<' );
		if ( element_start == NULL )
		{
			break;
		}

		++element_start;

		// Find the end of the HTML element.
		element_end = FindCharExcludeAttributeValues( element_start, NULL, '>' );
		if ( element_end == NULL )
		{
			break;
		}

		// See if the length of the element is enough for a form tag with attributes.
		if ( ( element_end - element_start ) >= 6 ) // It should have at least: "a href"
		{
			// We expect the anchor to have attributes.
			if ( element_start[ 0 ] == 'a' || element_start[ 0 ] == 'A' )
			{
				++element_start;

				attribute_name_start = element_start;

				// Locate each attribute.
				while ( attribute_name_start < element_end )
				{
					// Skip whitespace that appear before the attribute name.
					while ( attribute_name_start < element_end )
					{
						if ( attribute_name_start[ 0 ] != ' ' &&
							 attribute_name_start[ 0 ] != '\t' &&
							 attribute_name_start[ 0 ] != '\f' &&
							 attribute_name_start[ 0 ] != '\r' &&
							 attribute_name_start[ 0 ] != '\n' )
						{
							break;
						}

						++attribute_name_start;
					}

					// If these pointers are equal, then there was no whitespace after the element name. We want: "a "
					if ( attribute_name_start == element_start )
					{
						break;
					}

					// Each attribute value will begin with an "=".
					attribute_value_start = FindCharExcludeAttributeValues( attribute_name_start, element_end, '=' );
					if ( attribute_value_start == NULL )
					{
						break;
					}
					++attribute_value_start;

					// Skip whitespace that could appear after the attribute name, but before the "=".
					attribute_name_end = attribute_value_start - 1;
					while ( ( attribute_name_end - 1 ) >= attribute_name_start )
					{
						if ( *( attribute_name_end - 1 ) != ' ' &&
							 *( attribute_name_end - 1 ) != '\t' &&
							 *( attribute_name_end - 1 ) != '\f' &&
							 *( attribute_name_end - 1 ) != '\r' &&
							 *( attribute_name_end - 1 ) != '\n' )
						{
							break;
						}

						--attribute_name_end;
					}

					if ( ( attribute_name_end - attribute_name_start ) == 4 && _StrCmpNIA( attribute_name_start, "href", 4 ) == 0 )	// We found the attribute.
					{
						// Skip whitespace that could appear after the "=", but before the attribute value.
						while ( attribute_value_start < element_end )
						{
							if ( attribute_value_start[ 0 ] != ' ' &&
								 attribute_value_start[ 0 ] != '\t' &&
								 attribute_value_start[ 0 ] != '\f' &&
								 attribute_value_start[ 0 ] != '\r' &&
								 attribute_value_start[ 0 ] != '\n' )
							{
								break;
							}

							++attribute_value_start;
						}

						href = attribute_value_start;
					}

					// Find the end of the attribute value.
					char delimiter = attribute_value_start[ 0 ];
					if ( delimiter == '\"' || delimiter == '\'' )
					{
						++attribute_value_start;

						attribute_value_end = _StrChrA( attribute_value_start, delimiter );	// Find the end of the ID.
						if ( attribute_value_end == NULL || attribute_value_end >= element_end )
						{
							break;
						}
					}
					else	// No delimiter.
					{
						attribute_value_end = attribute_value_start;

						while ( attribute_value_end < element_end )
						{
							if ( attribute_value_end[ 0 ] == ' ' || attribute_value_end[ 0 ] == '\t' || attribute_value_end[ 0 ] == '\f' )
							{
								break;
							}

							++attribute_value_end;
						}
					}

					if ( href == NULL )
					{
						attribute_name_start = attribute_value_end + 1;
					}
					else// if ( href != NULL )
					{
						href = attribute_value_start;
						href_end = attribute_value_end;

						char *url_end = NULL;
						bool append_directory = false;

						int url_length = 0;

						// See if the url starts with the http(s) protocol.
						if ( ( ( href_end - href ) > 7 && _StrCmpNIA( href, "http://", 7 ) == 0 ) ||
							 ( ( href_end - href ) > 8 && _StrCmpNIA( href, "https://", 8 ) == 0 ) ||
							 ( ( href_end - href ) > 6 && _StrCmpNIA( href, "ftp://", 6 ) == 0 ) ||
							 ( ( href_end - href ) > 7 && _StrCmpNIA( href, "ftps://", 7 ) == 0 ) ||
							 ( ( href_end - href ) > 8 && _StrCmpNIA( href, "ftpes://", 8 ) == 0 ) ||
							 ( ( href_end - href ) > 7 && _StrCmpNIA( href, "sftp://", 7 ) == 0 ) )
						{
							url_length = ( int )( href_end - href );
						}
						else	// A relative url or unsupported protocol.
						{
							// See if the url is relative and should be appended to the root (end of the domain).
							if ( href[ 0 ] == '/' )
							{
								if ( source_url_start != NULL && source_url_root_end != NULL )
								{
									url_length = ( int )( ( source_url_root_end - source_url_start ) + ( href_end - href ) );

									url_end = source_url_root_end;
								}
							}
							else	// See if the url is an unsupported protocol. If not, then use it as a relative url.
							{
								char *protocol = href;

								// Make sure the first value of the url is a letter.
								// If it's not, then we'll consider the url a relative path.
								if ( ( *protocol >= 'a' && *protocol <= 'z' ) ||
									 ( *protocol >= 'A' && *protocol <= 'Z' ) )
								{
									++protocol;

									while ( protocol < href_end )
									{
										// If we found a colon before any non letter, number, '+', '-', or '.', then it's a valid protocol of some sort and we're not going to use it.
										if ( *protocol == ':' )
										{
											protocol = NULL;	// Don't append the url.

											break;
										}
										else if ( !( ( *protocol >= 'a' && *protocol <= 'z' ) ||
													 ( *protocol >= 'A' && *protocol <= 'Z' ) ||
													 ( *protocol >= '0' && *protocol <= '9' ) ||
													   *protocol == '+' ||
													   *protocol == '-' ||
													   *protocol == '.' ) )
										{
											break;	// A non letter, number, '+', '-', or '.' was found. We'll consider the url a relative path.
										}

										++protocol;
									}
								}

								if ( protocol != NULL )
								{
									// The relative url should be appended to the last directory.
									if ( source_url_start != NULL && source_url_end != NULL )
									{
										url_length = ( int )( ( source_url_end - source_url_start ) + ( href_end - href ) ) + 1;

										append_directory = true;

										url_end = source_url_end;
									}
								}
							}
						}

						if ( url_length > 0 )
						{
							if ( url_buffer != NULL )
							{
								if ( ( url_buffer_length + ( url_length + 2 ) ) >= url_buffer_size )
								{
									url_buffer_size += 8192;

									char *reallocated_url_buffer = ( char * )GlobalReAlloc( url_buffer, sizeof( char ) * url_buffer_size, GMEM_MOVEABLE );
									if ( reallocated_url_buffer == NULL )
									{
										goto CLEANUP;
									}

									url_buffer = reallocated_url_buffer;
								}
							}
							else
							{
								url_buffer = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( url_buffer_size + 1 ) );	// Include the NULL character.
							}

							if ( url_buffer != NULL )
							{
								if ( url_buffer_length > 0 )
								{
									url_buffer[ url_buffer_length++ ] = '\r';
									url_buffer[ url_buffer_length++ ] = '\n';
								}

								// Use the source url so that any relative url will form a full url.
								if ( url_end != NULL )
								{
									_memcpy_s( url_buffer + url_buffer_length, url_buffer_size - url_buffer_length, source_url_start, url_end - source_url_start );
									url_buffer_length += ( unsigned int )( url_end - source_url_start );

									if ( append_directory )
									{
										url_buffer[ url_buffer_length++ ] = '/';
									}
								}

								unsigned int decoded_url_length = 0;
								char *decoded_url = html_entity_decode_a( href, ( unsigned int )( href_end - href ), &decoded_url_length );

								_memcpy_s( url_buffer + url_buffer_length, url_buffer_size - url_buffer_length, decoded_url, decoded_url_length );
								url_buffer_length += decoded_url_length;

								GlobalFree( decoded_url );
							}
							else
							{
								goto CLEANUP;
							}
						}

						href = NULL;	// Reset so we can find the next url.

						break;
					}
				}
			}
		}
	}

CLEANUP:

	if ( url_buffer != NULL )
	{
		url_buffer[ url_buffer_length ] = 0;	// Sanity.

		int w_url_buffer_length = MultiByteToWideChar( CP_UTF8, 0, url_buffer, url_buffer_length + 1, NULL, 0 );	// Include the NULL terminator.
		w_url_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * w_url_buffer_length );
		MultiByteToWideChar( CP_UTF8, 0, url_buffer, url_buffer_length + 1, w_url_buffer, w_url_buffer_length );

		GlobalFree( url_buffer );
	}

	return w_url_buffer;
}

void PositionCursor( HWND hWnd, POINTL pt )
{
	_ScreenToClient( hWnd, ( POINT * )&pt );

	// Get the caret position along the vertical axis.
	WORD cursor_pos = LOWORD( _SendMessageW( hWnd, EM_CHARFROMPOS, 0, MAKELPARAM( 0, pt.y ) ) );

	if ( cursor_pos < 65355 )
	{
		_SendMessageW( hWnd, EM_SETSEL, cursor_pos, cursor_pos );
		_SendMessageW( hWnd, EM_SCROLLCARET, 0, 0 );
	}
}

ULONG STDMETHODCALLTYPE IDropTarget_AddRef( IDropTarget *This )
{
	return InterlockedIncrement( &( ( _IDropTarget * )This )->m_lRefCount );
}

HRESULT STDMETHODCALLTYPE IDropTarget_QueryInterface( IDropTarget *This, REFIID riid, void **ppvObject )
{
	if ( ppvObject == NULL )
	{
		return E_POINTER;
	}
	else
	{
		if ( _memcmp( &riid, &_IID_IUnknown, sizeof( GUID ) ) == 0 || _memcmp( &riid, &_IID_IDropTarget, sizeof( GUID ) ) == 0 )
		{
			IDropTarget_AddRef( This );

			*ppvObject = This;

			return S_OK;
		}
		else
		{
			*ppvObject = NULL;

			return E_NOINTERFACE;
		}
	}
}

ULONG STDMETHODCALLTYPE IDropTarget_Release( IDropTarget *This )
{
	_IDropTarget *_This = ( _IDropTarget * )This;

	LONG count = InterlockedDecrement( &_This->m_lRefCount );

	if ( count == 0 )
	{
		GlobalFree( _This );
	}

	return count;
}

HRESULT STDMETHODCALLTYPE IDropTarget_DragEnter( IDropTarget *This, IDataObject *pDataObj, DWORD /*grfKeyState*/, POINTL pt, DWORD *pdwEffect )
{
	_IDropTarget *_This = ( _IDropTarget * )This;

	_This->m_ClipFormat = 0;

	FORMATETC fetc;
	fetc.ptd = NULL;
	fetc.dwAspect = DVASPECT_CONTENT;
	fetc.lindex = -1;
	fetc.tymed = TYMED_HGLOBAL;

	UINT clip_formats[ 5 ] = { CF_HTML, CF_UNICODETEXT, CF_HDROP, CF_TEXT, CF_TREELISTVIEW };
	for ( char i = 0; i < 5; ++i )
	{
		fetc.cfFormat = ( CLIPFORMAT )clip_formats[ i ];
		if ( pDataObj->QueryGetData( &fetc ) == S_OK )
		{
			_This->m_ClipFormat = clip_formats[ i ];

			break;
		}
	}

	*pdwEffect = DROPEFFECT_NONE;

	if ( _This->m_ClipFormat == CF_TREELISTVIEW )
	{
		if ( _This->m_hWnd == g_hWnd_categories )
		{
			_SetFocus( g_hWnd_categories );

			g_drag_and_drop_cti = NULL;

			*pdwEffect = DROPEFFECT_MOVE;
		}
	}
	else if ( _This->m_ClipFormat != 0 )
	{
		_SetFocus( _This->m_hWnd );

		PositionCursor( _This->m_hWnd, pt );

		if ( _This->m_hWnd == g_hWnd_url_drop_window )
		{
			_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, 0xFF, LWA_ALPHA );
		}

		*pdwEffect = DROPEFFECT_COPY;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTarget_DragOver( IDropTarget *This, DWORD /*grfKeyState*/, POINTL pt, DWORD *pdwEffect )
{
	_IDropTarget *_This = ( _IDropTarget * )This;

	*pdwEffect = DROPEFFECT_NONE;

	if ( _This->m_ClipFormat == CF_TREELISTVIEW )
	{
		if ( _This->m_hWnd == g_hWnd_categories )
		{
			POINT p;
			_GetCursorPos( &p );

			TVHITTESTINFO tvht;
			_memzero( &tvht, sizeof( TVHITTESTINFO ) );
			tvht.pt.x = p.x;
			tvht.pt.y = p.y;
			_ScreenToClient( g_hWnd_categories, &tvht.pt );
			HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_HITTEST, 0, ( LPARAM )&tvht );
			if ( hti != NULL )
			{
				TVITEM tvi;
				_memzero( &tvi, sizeof( TVITEM ) );
				tvi.mask = TVIF_PARAM;
				tvi.hItem = tvht.hItem;
				_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvi );

				DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
				if ( dll_node != NULL )
				{
					CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
					if ( cti != NULL &&
					   ( cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO ||
						 cti->type == CATEGORY_TREE_INFO_TYPE_STATUS && cti->data == NULL ) )
					{
						g_drag_and_drop_cti = cti;

						*pdwEffect = DROPEFFECT_MOVE;
					}
				}
			}

			_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_DROPHILITE, ( LPARAM )hti );
			_SetFocus( g_hWnd_categories );
		}
	}
	else if ( _This->m_ClipFormat != 0 )
	{
		PositionCursor( _This->m_hWnd, pt );

		*pdwEffect = DROPEFFECT_COPY;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTarget_DragLeave( IDropTarget *This )
{
	_IDropTarget *_This = ( _IDropTarget * )This;

	if ( _This->m_hWnd == g_hWnd_categories )
	{
		g_drag_and_drop_cti = NULL;

		_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_DROPHILITE, NULL );
	}
	else if ( _This->m_hWnd == g_hWnd_url_drop_window )
	{
		_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropTarget_Drop( IDropTarget *This, IDataObject *pDataObj, DWORD /*grfKeyState*/, POINTL pt, DWORD *pdwEffect )
{
	_IDropTarget *_This = ( _IDropTarget * )This;

	*pdwEffect = DROPEFFECT_NONE;

	if ( _This->m_ClipFormat == CF_TREELISTVIEW )
	{
		_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_DROPHILITE, NULL );

		*pdwEffect = DROPEFFECT_MOVE;
	}
	else if ( _This->m_ClipFormat != 0 )
	{
		PositionCursor( _This->m_hWnd, pt );

		FORMATETC fetc;
		fetc.cfFormat = ( CLIPFORMAT )_This->m_ClipFormat;
		fetc.ptd = NULL;
		fetc.dwAspect = DVASPECT_CONTENT;
		fetc.lindex = -1;
		fetc.tymed = TYMED_HGLOBAL;

		STGMEDIUM stgm;

		if ( pDataObj->GetData( &fetc, &stgm ) == S_OK )
		{
			PVOID data = GlobalLock( stgm.hGlobal );

			if ( data != NULL && _This->m_ClipFormat == CF_HTML )
			{
				// Reallocate the data buffer since it doesn't include a NULL terminator. (STUPID!!!)
				SIZE_T data_size = GlobalSize( stgm.hGlobal );

				HGLOBAL reallocated_data = GlobalReAlloc( data, data_size + sizeof( char ), GMEM_ZEROINIT );
				if ( reallocated_data != NULL )
				{
					data = reallocated_data;
				}
				else
				{
					char *t_data = ( char * )data;
					t_data[ ( data_size > 0 ? ( data_size - 1 ) : 0 ) ] = 0;
				}

				// We can reuse the data pointer since it's original pointer is in stgm and freed with ReleaseStgMedium.
				// If ParseHTMLClipboard allocates memory, then we'll free it with the GlobalFree below.
				data = ParseHTMLClipboard( ( char * )data );

				if ( data == NULL )
				{
					GlobalUnlock( stgm.hGlobal );

					_ReleaseStgMedium( &stgm );

					_This->m_ClipFormat = fetc.cfFormat = CF_UNICODETEXT;

					if ( pDataObj->GetData( &fetc, &stgm ) == S_OK )
					{
						data = GlobalLock( stgm.hGlobal );
					}
					else
					{
						_This->m_ClipFormat = fetc.cfFormat = CF_TEXT;

						if ( pDataObj->GetData( &fetc, &stgm ) == S_OK )
						{
							data = GlobalLock( stgm.hGlobal );
						}
					}
				}
			}

			if ( data != NULL )
			{
				if ( _This->m_ClipFormat == CF_HDROP )
				{
					HandleFileList( ( HDROP )data );
				}
				else
				{
					// g_hWnd_add_urls might be NULL. We just want to make sure we're not dropping something into its URL edit control (g_hWnd_edit_add).
					if ( cfg_drag_and_drop_action != DRAG_AND_DROP_ACTION_NONE && ( g_hWnd_add_urls == NULL || _GetParent( _This->m_hWnd ) != g_hWnd_add_urls ) )
					{
						HandleAddInfo( _This->m_ClipFormat, data );
					}
					else
					{
						_SendMessageW( ( g_hWnd_add_urls != NULL ? g_hWnd_add_urls : g_hWnd_main ), WM_PROPAGATE, _This->m_ClipFormat, ( LPARAM )data );
					}
				}

				if ( _This->m_ClipFormat == CF_HTML )
				{
					GlobalFree( data );
				}
			}

			GlobalUnlock( stgm.hGlobal );

			_ReleaseStgMedium( &stgm );
		}

		*pdwEffect = DROPEFFECT_COPY;
	}

	return S_OK;
}

struct IDropTargetVtbl Vtbl =
{
	IDropTarget_QueryInterface,
	IDropTarget_AddRef,
	IDropTarget_Release,
	IDropTarget_DragEnter,
	IDropTarget_DragOver,
	IDropTarget_DragLeave,
	IDropTarget_Drop
};

_IDropTarget *Create_IDropTarget( HWND hWnd )
{
	_IDropTarget *DropTarget = ( _IDropTarget * )GlobalAlloc( GMEM_FIXED, sizeof( _IDropTarget ) );

	if ( DropTarget != NULL )
	{
		DropTarget->lpVtbl = &Vtbl;

		DropTarget->m_hWnd = hWnd;
		DropTarget->m_lRefCount = 1;
		DropTarget->m_ClipFormat = 0;
	}

	return DropTarget;
}

void RegisterDropWindow( HWND hWnd, IDropTarget **DropTarget )
{
	if ( DropTarget != NULL )
	{
		*DropTarget = ( IDropTarget * )Create_IDropTarget( hWnd );

		if ( *DropTarget != NULL )
		{
			_CoLockObjectExternal( ( struct IUnknown * )( *DropTarget ), TRUE, FALSE );

			_RegisterDragDrop( hWnd, *DropTarget );
		}
	}
}

void UnregisterDropWindow( HWND hWnd, IDropTarget *DropTarget )
{
	_RevokeDragDrop( hWnd );

	if ( DropTarget != NULL )
	{
		_CoLockObjectExternal( ( struct IUnknown * )DropTarget, FALSE, TRUE );

		DropTarget->Release();
	}
}

///////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE IDataObject_AddRef( IDataObject *This )
{
	return InterlockedIncrement( &( ( _IDataObject * )This )->m_lRefCount );
}

HRESULT STDMETHODCALLTYPE IDataObject_QueryInterface( IDataObject *This, REFIID riid, void **ppvObject )
{
	if ( ppvObject == NULL )
	{
		return E_POINTER;
	}
	else
	{
		if ( _memcmp( &riid, &_IID_IUnknown, sizeof( GUID ) ) == 0 || _memcmp( &riid, &_IID_IDataObject, sizeof( GUID ) ) == 0 )
		{
			IDataObject_AddRef( This );

			*ppvObject = This;

			return S_OK;
		}
		else
		{
			*ppvObject = NULL;

			return E_NOINTERFACE;
		}
	}
}

ULONG STDMETHODCALLTYPE IDataObject_Release( IDataObject *This )
{
	_IDataObject *_This = ( _IDataObject * )This;

	LONG count = InterlockedDecrement( &_This->m_lRefCount );

	if ( count == 0 )
	{
		GlobalFree( _This );
	}

	return count;
}

int LookupFormatEtc( _IDataObject *pDataObj, FORMATETC *pFormatEtc )
{
	for ( int i = 0; i < pDataObj->m_nNumFormats; ++i )
	{
		if ( ( pDataObj->m_pFormatEtc[ i ].tymed & pFormatEtc->tymed ) &&
			   pDataObj->m_pFormatEtc[ i ].cfFormat == pFormatEtc->cfFormat &&
			   pDataObj->m_pFormatEtc[ i ].dwAspect == pFormatEtc->dwAspect )
		{
			return i;
		}
	}

	return -1;
}

HRESULT STDMETHODCALLTYPE IDataObject_QueryGetData( IDataObject *This, FORMATETC *pformatetc )
{
	return LookupFormatEtc( ( _IDataObject * )This, pformatetc ) == -1 ? DV_E_FORMATETC : S_OK;
}

HRESULT STDMETHODCALLTYPE IDataObject_GetData( IDataObject *This, FORMATETC *pformatetcIn, STGMEDIUM *pmedium )
{
	_IDataObject *_This = ( _IDataObject * )This;

	int i;

	// Match the specified FORMATETC with one of our supported formats.
	if ( ( i = LookupFormatEtc( _This, pformatetcIn ) ) == -1 )
	{
		return DV_E_FORMATETC;
	}

	// Transfer data into supplied storage medium.
	pmedium->tymed = _This->m_pFormatEtc[ i ].tymed;
	pmedium->pUnkForRelease = 0;

	// Copy the data into the caller's storage medium.
	switch ( _This->m_pFormatEtc[ i ].tymed )
	{
		case TYMED_HGLOBAL:
		{
			SIZE_T len = GlobalSize( _This->m_pStgMedium[ i ].hGlobal );
			PVOID source = GlobalLock( _This->m_pStgMedium[ i ].hGlobal );

			pmedium->hGlobal = GlobalAlloc( GMEM_FIXED, len );

			_memcpy_s( pmedium->hGlobal, len, source, len );

			GlobalUnlock( _This->m_pStgMedium[ i ].hGlobal );
		}
		break;

		default:
		{
			return DV_E_FORMATETC;
		}
		break;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDataObject_EnumFormatEtc( IDataObject *This, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc )
{
	_IDataObject *_This = ( _IDataObject * )This;

	// Only the get direction is supported for OLE.
	if ( dwDirection == DATADIR_GET )
	{
		return _SHCreateStdEnumFmtEtc( _This->m_nNumFormats, _This->m_pFormatEtc, ppenumFormatEtc );
	}
	else
	{
		// The direction specified is not supported for drag and drop.
		return E_NOTIMPL;
	}
}

HRESULT STDMETHODCALLTYPE IDataObject_DAdvise( IDataObject * /*This*/, FORMATETC * /*pformatetc*/, DWORD /*advf*/, IAdviseSink * /*pAdvSink*/, DWORD * /*pdwConnection*/ )
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE IDataObject_DUnadvise( IDataObject * /*This*/, DWORD /*dwConnection*/ )
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE IDataObject_EnumDAdvise( IDataObject * /*This*/, IEnumSTATDATA ** /*ppenumAdvise*/ )
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE IDataObject_GetDataHere( IDataObject * /*This*/, FORMATETC * /*pformatetc*/, STGMEDIUM * /*pmedium*/ )
{
	return DATA_E_FORMATETC;
}

HRESULT STDMETHODCALLTYPE IDataObject_GetCanonicalFormatEtc( IDataObject * /*This*/, FORMATETC * /*pformatectIn*/, FORMATETC *pformatetcOut )
{
	// Apparently we have to set this field to NULL even though we don't do anything else.
	pformatetcOut->ptd = NULL;

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDataObject_SetData( IDataObject * /*This*/, FORMATETC * /*pformatetc*/, STGMEDIUM * /*pmedium */, BOOL /*fRelease*/ )
{
	return E_NOTIMPL;
}

struct IDataObjectVtbl DataVtbl =
{
	IDataObject_QueryInterface,
	IDataObject_AddRef,
	IDataObject_Release,
	IDataObject_GetData,
	IDataObject_GetDataHere,
	IDataObject_QueryGetData,
	IDataObject_GetCanonicalFormatEtc,
	IDataObject_SetData,
	IDataObject_EnumFormatEtc,
	IDataObject_DAdvise,
	IDataObject_DUnadvise,
	IDataObject_EnumDAdvise
};

_IDataObject *Create_IDataObject( FORMATETC *fmtetc, STGMEDIUM *stgmed, int count )
{
	_IDataObject *DataObject = ( _IDataObject * )GlobalAlloc( GMEM_FIXED, sizeof( _IDataObject ) );

	if ( DataObject != NULL )
	{
		DataObject->lpVtbl = &DataVtbl;

		DataObject->m_lRefCount = 1;
		DataObject->m_nNumFormats = count;

		DataObject->m_pFormatEtc = ( FORMATETC * )GlobalAlloc( GMEM_FIXED, sizeof( FORMATETC ) * count );
		DataObject->m_pStgMedium = ( STGMEDIUM * )GlobalAlloc( GMEM_FIXED, sizeof( STGMEDIUM ) * count );

		for ( int i = 0; i < count; ++i )
		{
			_memcpy_s( DataObject->m_pFormatEtc + i, sizeof( FORMATETC ), fmtetc + i, sizeof( FORMATETC ) );
			_memcpy_s( DataObject->m_pStgMedium + i, sizeof( STGMEDIUM ), stgmed + i, sizeof( STGMEDIUM ) );
		}
	}

	return DataObject;
}

///////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE IDropSource_AddRef( IDropSource *This )
{
	return InterlockedIncrement( &( ( _IDropSource * )This )->m_lRefCount );
}

HRESULT STDMETHODCALLTYPE IDropSource_QueryInterface( IDropSource *This, REFIID riid, void **ppvObject )
{
	if ( ppvObject == NULL )
	{
		return E_POINTER;
	}
	else
	{
		if ( _memcmp( &riid, &_IID_IUnknown, sizeof( GUID ) ) == 0 || _memcmp( &riid, &_IID_IDropSource, sizeof( GUID ) ) == 0 )
		{
			IDropSource_AddRef( This );

			*ppvObject = This;

			return S_OK;
		}
		else
		{
			*ppvObject = NULL;

			return E_NOINTERFACE;
		}
	}
}

ULONG STDMETHODCALLTYPE IDropSource_Release( IDropSource *This )
{
	_IDropSource *_This = ( _IDropSource * )This;

	LONG count = InterlockedDecrement( &_This->m_lRefCount );

	if ( count == 0 )
	{
		GlobalFree( _This );
	}

	return count;
}

HRESULT STDMETHODCALLTYPE IDropSource_QueryContinueDrag( IDropSource * /*This*/, BOOL fEscapePressed, DWORD grfKeyState )
{
	if ( fEscapePressed == TRUE )
	{
		return DRAGDROP_S_CANCEL;
	}

	if ( ( grfKeyState & MK_LBUTTON ) == 0 )
	{
		return DRAGDROP_S_DROP;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDropSource_GiveFeedback( IDropSource * /*This*/, DWORD /*dwEffect*/ )
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

struct IDropSourceVtbl SourceVtbl =
{
	IDropSource_QueryInterface,
	IDropSource_AddRef,
	IDropSource_Release,
	IDropSource_QueryContinueDrag,
	IDropSource_GiveFeedback
};

_IDropSource *Create_IDropSource( HWND hWnd )
{
	_IDropSource *DropSource = ( _IDropSource * )GlobalAlloc( GMEM_FIXED, sizeof( _IDropSource ) );

	if ( DropSource != NULL )
	{
		DropSource->lpVtbl = &SourceVtbl;

		DropSource->m_hWnd = hWnd;
		DropSource->m_lRefCount = 1;
	}

	return DropSource;
}
