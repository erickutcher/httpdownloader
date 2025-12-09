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

#include "lite_shell32.h"
#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_normaliz.h"

#include "file_operations.h"
#include "menus.h"
#include "utilities.h"
#include "string_tables.h"

#include "ftp_parsing.h"
#include "connection.h"

#include "categories.h"
#include "treelistview.h"

char read_config()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\http_downloader_settings\0", 26 );
	//g_base_directory[ g_base_directory_length + 25 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading configuration: %S", g_base_directory );
#endif

	HANDLE hFile_cfg = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_cfg = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_cfg, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

#ifdef ENABLE_LOGGING
		lfz = fz;
#endif

		unsigned char i;
		int reserved;

		// Our config file is going to be small. If it's something else, we're not going to read it.
		if ( fz >= 1024 && fz < 10240 )
		{
			unsigned char magic_identifier[ 4 ];
			BOOL bRet = ReadFile( hFile_cfg, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
			if ( bRet != FALSE )
			{
				unsigned char version = magic_identifier[ 3 ];// - 0x00;

				if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_SETTINGS, 3 ) == 0 && version <= 0x0F )
				{
					fz -= 4;

					char *cfg_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );
					if ( cfg_buf != NULL )
					{
						BOOL bRet = ReadFile( hFile_cfg, cfg_buf, sizeof( char ) * fz, &read, NULL );

						// Read the config. It must be in the order specified below.
						if ( bRet != FALSE && read == fz )
						{
							cfg_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

							reserved = 1024 - ( version < 0x09 ? 721 : 749 );

							char *next = cfg_buf;

							// Main Window

							_memcpy_s( &cfg_pos_x, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_pos_y, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_width, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_height, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );

							_memcpy_s( &cfg_min_max, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							if ( version >= 0x09 )
							{
								for ( i = 0; i < NUM_COLUMNS; ++i )
								{
									_memcpy_s( download_columns_width[ i ], sizeof( int ), next, sizeof( int ) );
									next += sizeof( int );
								}

								for ( i = 0; i < NUM_COLUMNS; ++i )
								{
									_memcpy_s( download_columns[ i ], sizeof( char ), next, sizeof( char ) );
									next += sizeof( char );
								}
							}
							else	// Reset for older versions.
							{
								next += ( ( sizeof( int ) + sizeof( char ) ) * ( NUM_COLUMNS - 2 ) );
							}

							_memcpy_s( &cfg_show_column_headers, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_sorted_column_index, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );

							_memcpy_s( &cfg_sorted_direction, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_show_toolbar, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_show_status_bar, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_show_categories, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );

								_memcpy_s( &cfg_splitter_pos_x, sizeof( int ), next, sizeof( int ) );
								next += sizeof( int );
							}

							_memcpy_s( &cfg_t_down_speed, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_t_downloaded, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_t_file_size, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_t_speed_limit, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_t_status_downloaded, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_t_status_down_speed, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_t_status_speed_limit, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							// Global Speed Limit

							_memcpy_s( &cfg_download_speed_limit, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
							next += sizeof( unsigned long long );

							//

							_memcpy_s( &cfg_total_downloaded, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
							next += sizeof( unsigned long long );

							// Options General

							_memcpy_s( &cfg_tray_icon, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_minimize_to_tray, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_close_to_tray, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_start_in_tray, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_show_notification, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_show_tray_progress, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_always_on_top, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_check_for_updates, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_enable_drop_window, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_drop_window_transparency, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_show_drop_window_progress, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_drop_pos_x, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_drop_pos_y, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );

							_memcpy_s( &cfg_play_sound, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_play_sound_fail, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );
							}

							// Options Appearance

							_memcpy_s( &cfg_show_gridlines, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_draw_full_rows, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_draw_all_rows, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_show_part_progress, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_sort_added_and_updating_items, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_expand_added_group_items, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_scroll_to_last_item, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_show_embedded_icon, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );
							}

							_memcpy_s( &cfg_background_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_gridline_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_selection_marquee_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );

							_memcpy_s( &cfg_odd_row_font_settings.font_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_odd_row_font_settings.lf.lfHeight, sizeof( LONG ), next, sizeof( LONG ) );
							next += sizeof( LONG );
							_memcpy_s( &cfg_odd_row_font_settings.lf.lfWeight, sizeof( LONG ), next, sizeof( LONG ) );
							next += sizeof( LONG );
							_memcpy_s( &cfg_odd_row_font_settings.lf.lfItalic, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );
							_memcpy_s( &cfg_odd_row_font_settings.lf.lfUnderline, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );
							_memcpy_s( &cfg_odd_row_font_settings.lf.lfStrikeOut, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );

							_memcpy_s( &cfg_odd_row_background_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_odd_row_highlight_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_odd_row_highlight_font_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );

							_memcpy_s( &cfg_even_row_font_settings.font_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_even_row_font_settings.lf.lfHeight, sizeof( LONG ), next, sizeof( LONG ) );
							next += sizeof( LONG );
							_memcpy_s( &cfg_even_row_font_settings.lf.lfWeight, sizeof( LONG ), next, sizeof( LONG ) );
							next += sizeof( LONG );
							_memcpy_s( &cfg_even_row_font_settings.lf.lfItalic, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );
							_memcpy_s( &cfg_even_row_font_settings.lf.lfUnderline, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );
							_memcpy_s( &cfg_even_row_font_settings.lf.lfStrikeOut, sizeof( BYTE ), next, sizeof( BYTE ) );
							next += sizeof( BYTE );

							_memcpy_s( &cfg_even_row_background_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_even_row_highlight_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );
							_memcpy_s( &cfg_even_row_highlight_font_color, sizeof( COLORREF ), next, sizeof( COLORREF ) );
							next += sizeof( COLORREF );

							for ( i = 0; i < NUM_COLORS; ++i )
							{
								_memcpy_s( progress_colors[ i ], sizeof( COLORREF ), next, sizeof( COLORREF ) );
								next += sizeof( COLORREF );
							}

							// Tray and URL Drop window progress colors.

							for ( i = 0; i < TD_NUM_COLORS; ++i )
							{
								_memcpy_s( td_progress_colors[ i ], sizeof( COLORREF ), next, sizeof( COLORREF ) );
								next += sizeof( COLORREF );
							}

							// Options Connection

							_memcpy_s( &cfg_max_downloads, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_default_download_parts, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_retry_downloads_count, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_retry_parts_count, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_timeout, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							_memcpy_s( &cfg_max_redirects, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_default_speed_limit, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
							next += sizeof( unsigned long long );

							_memcpy_s( &cfg_default_ssl_version, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							// Options FTP

							_memcpy_s( &cfg_ftp_mode_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_ftp_enable_fallback_mode, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_ftp_address_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_ftp_ip_address, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							_memcpy_s( &cfg_ftp_port_start, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							_memcpy_s( &cfg_ftp_port_end, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							_memcpy_s( &cfg_ftp_send_keep_alive, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							// Options Proxy

							_memcpy_s( &cfg_enable_proxy, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_address_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_ip_address, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							_memcpy_s( &cfg_port, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							//

							_memcpy_s( &cfg_enable_proxy_s, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_address_type_s, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_ip_address_s, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							_memcpy_s( &cfg_port_s, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							//

							_memcpy_s( &cfg_enable_proxy_socks, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_socks_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_address_type_socks, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_ip_address_socks, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							_memcpy_s( &cfg_port_socks, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							_memcpy_s( &cfg_use_authentication_socks, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_resolve_domain_names_v4a, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_resolve_domain_names, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							// Options Server

							_memcpy_s( &cfg_enable_server, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_server_address_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_server_ip_address, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							_memcpy_s( &cfg_server_port, sizeof( unsigned short ), next, sizeof( unsigned short ) );
							next += sizeof( unsigned short );

							_memcpy_s( &cfg_use_authentication, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_authentication_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_server_enable_ssl, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_certificate_type, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_server_ssl_version, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_show_remote_connection_notification, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );
							}

							// Options Advanced

							_memcpy_s( &cfg_enable_download_history, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_enable_quick_allocation, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_set_filetime, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_update_redirected, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_use_one_instance, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_prevent_standby, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_resume_downloads, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_drag_and_drop_action, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_prompt_last_modified, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_prompt_rename, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_prompt_file_size, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );
							_memcpy_s( &cfg_max_file_size, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
							next += sizeof( unsigned long long );

							_memcpy_s( &cfg_shutdown_action, sizeof( unsigned char ), next, sizeof( unsigned char ) );
							next += sizeof( unsigned char );

							_memcpy_s( &cfg_use_temp_download_directory, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_thread_count, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							// Options SFTP

							_memcpy_s( &cfg_sftp_enable_compression, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_sftp_attempt_gssapi_authentication, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );
							_memcpy_s( &cfg_sftp_attempt_gssapi_key_exchange, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_sftp_keep_alive_time, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_sftp_rekey_time, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_sftp_gss_rekey_time, sizeof( int ), next, sizeof( int ) );
							next += sizeof( int );
							_memcpy_s( &cfg_sftp_rekey_data_limit, sizeof( unsigned long ), next, sizeof( unsigned long ) );
							next += sizeof( unsigned long );

							for ( i = 0; i < KEX_ALGORITHM_COUNT; ++i )
							{
								_memcpy_s( &cfg_priority_kex_algorithm[ i ], sizeof( unsigned char ), next, sizeof( unsigned char ) );
								next += sizeof( unsigned char );
							}

							for ( i = 0; i < HOST_KEY_COUNT; ++i )
							{
								_memcpy_s( &cfg_priority_host_key[ i ], sizeof( unsigned char ), next, sizeof( unsigned char ) );
								next += sizeof( unsigned char );
							}

							for ( i = 0; i < ENCRYPTION_CIPHER_COUNT; ++i )
							{
								_memcpy_s( &cfg_priority_encryption_cipher[ i ], sizeof( unsigned char ), next, sizeof( unsigned char ) );
								next += sizeof( unsigned char );
							}

							//

							_memcpy_s( &cfg_reallocate_parts, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_reallocate_threshold_size, sizeof( unsigned long long ), next, sizeof( unsigned long long ) );
								next += sizeof( unsigned long long );
							}

							_memcpy_s( &cfg_download_non_200_206, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_move_to_trash, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_enable_sparse_file_allocation, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							_memcpy_s( &cfg_override_list_prompts, sizeof( bool ), next, sizeof( bool ) );
							next += sizeof( bool );

							if ( version >= 0x09 )
							{
								_memcpy_s( &cfg_apply_initial_proxy, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );

								_memcpy_s( &cfg_category_move, sizeof( bool ), next, sizeof( bool ) );
								next += sizeof( bool );
							}


							//


							next += reserved;	// Skip past reserved bytes.

							int string_length = 0;
							int cfg_val_length = 0;


							//


							// Options General

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								if ( string_length > 1 )
								{
									cfg_sound_file_path = UTF8StringToWideString( next, string_length );
								}

								next += string_length;
							}

							if ( version >= 0x09 )
							{
								if ( ( DWORD )( next - cfg_buf ) < read )
								{
									string_length = lstrlenA( next ) + 1;

									if ( string_length > 1 )
									{
										cfg_sound_fail_file_path = UTF8StringToWideString( next, string_length );
									}

									next += string_length;
								}
							}

							// Options Appearance

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								if ( string_length > 1 && string_length <= LF_FACESIZE )
								{
									cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
									MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_odd_row_font_settings.lf.lfFaceName, cfg_val_length );
								}

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								if ( string_length > 1 && string_length <= LF_FACESIZE )
								{
									cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
									MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_even_row_font_settings.lf.lfFaceName, cfg_val_length );
								}

								next += string_length;
							}

							// Options FTP

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_ftp_hostname = UTF8StringToWideString( next, string_length );

								next += string_length;
							}

							// Options Proxy

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_hostname = UTF8StringToWideString( next, string_length );

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_username, string_length, next, string_length );
										proxy_auth_username[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_username, string_length );

										// Read username.
										cfg_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

										GlobalFree( proxy_auth_username );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_password, string_length, next, string_length );
										proxy_auth_password[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_password, string_length );

										// Read password.
										cfg_proxy_auth_password = UTF8StringToWideString( proxy_auth_password, string_length + 1 );

										GlobalFree( proxy_auth_password );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							//

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_hostname_s = UTF8StringToWideString( next, string_length );

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_username_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_username_s, string_length, next, string_length );
										proxy_auth_username_s[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_username_s, string_length );

										// Read username.
										cfg_proxy_auth_username_s = UTF8StringToWideString( proxy_auth_username_s, string_length + 1 );

										GlobalFree( proxy_auth_username_s );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_password_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_password_s, string_length, next, string_length );
										proxy_auth_password_s[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_password_s, string_length );

										// Read password.
										cfg_proxy_auth_password_s = UTF8StringToWideString( proxy_auth_password_s, string_length + 1 );

										GlobalFree( proxy_auth_password_s );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							//

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_hostname_socks = UTF8StringToWideString( next, string_length );

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_ident_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_ident_username_socks, string_length, next, string_length );
										proxy_auth_ident_username_socks[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_ident_username_socks, string_length );

										// Read username.
										cfg_proxy_auth_ident_username_socks = UTF8StringToWideString( proxy_auth_ident_username_socks, string_length + 1 );

										GlobalFree( proxy_auth_ident_username_socks );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_username_socks, string_length, next, string_length );
										proxy_auth_username_socks[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_username_socks, string_length );

										// Read username.
										cfg_proxy_auth_username_socks = UTF8StringToWideString( proxy_auth_username_socks, string_length + 1 );

										GlobalFree( proxy_auth_username_socks );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *proxy_auth_password_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_password_socks, string_length, next, string_length );
										proxy_auth_password_socks[ string_length ] = 0; // Sanity.

										decode_cipher( proxy_auth_password_socks, string_length );

										// Read password.
										cfg_proxy_auth_password_socks = UTF8StringToWideString( proxy_auth_password_socks, string_length + 1 );

										GlobalFree( proxy_auth_password_socks );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							// Options Server

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_server_hostname = UTF8StringToWideString( next, string_length );

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *authentication_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( authentication_username, string_length, next, string_length );
										authentication_username[ string_length ] = 0; // Sanity.

										decode_cipher( authentication_username, string_length );

										// Read username.
										cfg_authentication_username = UTF8StringToWideString( authentication_username, string_length + 1 );

										GlobalFree( authentication_username );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										char *authentication_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( authentication_password, string_length, next, string_length );
										authentication_password[ string_length ] = 0; // Sanity.

										decode_cipher( authentication_password, string_length );

										// Read password.
										cfg_authentication_password = UTF8StringToWideString( authentication_password, string_length + 1 );

										GlobalFree( authentication_password );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								// Length of the string - not including the NULL character.
								_memcpy_s( &string_length, sizeof( unsigned short ), next, sizeof( unsigned short ) );
								next += sizeof( unsigned short );

								if ( string_length > 0 )
								{
									if ( ( ( DWORD )( next - cfg_buf ) + string_length < read ) )
									{
										// string_length does not contain the NULL character of the string.
										g_certificate_pkcs_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( g_certificate_pkcs_password, string_length, next, string_length );
										g_certificate_pkcs_password[ string_length ] = 0; // Sanity.

										decode_cipher( g_certificate_pkcs_password, string_length );

										// Read password.
										cfg_certificate_pkcs_password = UTF8StringToWideString( g_certificate_pkcs_password, string_length + 1 );

										next += string_length;
									}
									else
									{
										read = 0;
									}
								}
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_certificate_pkcs_file_name = UTF8StringToWideString( next, string_length );

								g_certificate_pkcs_file_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
								_memcpy_s( g_certificate_pkcs_file_name, string_length, next, string_length );
								g_certificate_pkcs_file_name[ string_length - 1 ] = 0; // Sanity.

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_certificate_cer_file_name = UTF8StringToWideString( next, string_length );

								g_certificate_cer_file_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
								_memcpy_s( g_certificate_cer_file_name, string_length, next, string_length );
								g_certificate_cer_file_name[ string_length - 1 ] = 0; // Sanity.

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								cfg_certificate_key_file_name = UTF8StringToWideString( next, string_length );

								g_certificate_key_file_name = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
								_memcpy_s( g_certificate_key_file_name, string_length, next, string_length );
								g_certificate_key_file_name[ string_length - 1 ] = 0; // Sanity.

								next += string_length;
							}

							// Options Advanced

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								if ( string_length > 1 )
								{
									cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
									cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
									MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_default_download_directory, cfg_val_length );

									g_default_download_directory_length = cfg_val_length - 1;	// Store the base directory length.
								}

								next += string_length;
							}

							if ( ( DWORD )( next - cfg_buf ) < read )
							{
								string_length = lstrlenA( next ) + 1;

								if ( string_length > 1 )
								{
									cfg_val_length = MultiByteToWideChar( CP_UTF8, 0, next, string_length, NULL, 0 );	// Include the NULL terminator.
									cfg_temp_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * cfg_val_length );
									MultiByteToWideChar( CP_UTF8, 0, next, string_length, cfg_temp_download_directory, cfg_val_length );

									g_temp_download_directory_length = cfg_val_length - 1;	// Store the base directory length.
								}

								next += string_length;
							}


							// Set the default values for bad configuration values.

							if ( cfg_splitter_pos_x < SPLITTER_WIDTH ) { cfg_splitter_pos_x = SPLITTER_POS_X; }

							CheckColumnOrders( download_columns, NUM_COLUMNS );

							// Revert column widths if they exceed our limits.
							CheckColumnWidths();

							if ( cfg_t_down_speed > SIZE_FORMAT_AUTO ) { cfg_t_down_speed = SIZE_FORMAT_AUTO; }
							if ( cfg_t_downloaded > SIZE_FORMAT_AUTO ) { cfg_t_downloaded = SIZE_FORMAT_AUTO; }
							if ( cfg_t_file_size > SIZE_FORMAT_AUTO ) { cfg_t_file_size = SIZE_FORMAT_AUTO; }
							if ( cfg_t_speed_limit > SIZE_FORMAT_AUTO ) { cfg_t_speed_limit = SIZE_FORMAT_AUTO; }
							if ( cfg_t_status_downloaded > SIZE_FORMAT_AUTO ) { cfg_t_status_downloaded = SIZE_FORMAT_AUTO; }
							if ( cfg_t_status_down_speed > SIZE_FORMAT_AUTO ) { cfg_t_status_down_speed = SIZE_FORMAT_AUTO; }
							if ( cfg_t_status_speed_limit > SIZE_FORMAT_AUTO ) { cfg_t_status_speed_limit = SIZE_FORMAT_AUTO; }

							if ( cfg_max_downloads > 100 )
							{
								cfg_max_downloads = 100;
							}
							/*else if ( cfg_max_downloads == 0 )
							{
								cfg_max_downloads = 1;
							}*/

							if ( cfg_retry_downloads_count > 100 ) { cfg_retry_downloads_count = 100; }
							if ( cfg_retry_parts_count > 100 ) { cfg_retry_parts_count = 100; }
							if ( cfg_timeout > 300 || ( cfg_timeout > 0 && cfg_timeout < 10 ) ) { cfg_timeout = 60; }

							if ( cfg_default_download_parts > 100 )
							{
								cfg_default_download_parts = 100;
							}
							else if ( cfg_default_download_parts == 0 )
							{
								cfg_default_download_parts = 1;
							}

							if ( cfg_max_redirects > 100 )
							{
								cfg_max_redirects = 100;
							}

							if ( cfg_reallocate_threshold_size < 1048576 )	// 1 MB
							{
								cfg_reallocate_threshold_size = 1048576;
							}

							if ( cfg_thread_count > g_max_threads )
							{
								cfg_thread_count = max( ( g_max_threads / 2 ), 1 );
							}
							else if ( cfg_thread_count == 0 )
							{
								cfg_thread_count = 1;
							}

							if ( g_can_use_tls_1_3 && cfg_default_ssl_version >= 5 ) { cfg_default_ssl_version = 5; }	// TLS 1.3.
							else if ( cfg_default_ssl_version > 4 ) { cfg_default_ssl_version = 4; }	// TLS 1.2.

							if ( cfg_server_port == 0 ) { cfg_server_port = 1; }
							if ( g_can_use_tls_1_3 && cfg_server_ssl_version >= 5 ) { cfg_server_ssl_version = 5; } // TLS 1.3.
							else if ( cfg_server_ssl_version > 4 ) { cfg_server_ssl_version = 4; } // TLS 1.2.
							if ( cfg_authentication_type != AUTH_TYPE_BASIC && cfg_authentication_type != AUTH_TYPE_DIGEST ) { cfg_authentication_type = AUTH_TYPE_BASIC; }

							if ( cfg_port == 0 ) { cfg_port = 1; }
							if ( cfg_port_s == 0 ) { cfg_port_s = 1; }
							if ( cfg_port_socks == 0 ) { cfg_port_socks = 1; }

							if ( cfg_max_file_size == 0 ) { cfg_max_file_size = MAX_FILE_SIZE; }

							if ( cfg_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN && !g_is_windows_8_or_higher )
							{
								cfg_shutdown_action = SHUTDOWN_ACTION_NONE;
							}
						}
						else
						{
							ret_status = -1;	// Can't open file for reading.
						}

						GlobalFree( cfg_buf );
					}
				}
				else
				{
					ret_status = -2;	// Bad file format.
				}
			}
			else
			{
				ret_status = -1;	// Can't open file for reading.
			}
		}
		else
		{
			ret_status = -3;	// Incorrect file size.
		}

		UnlockFileEx( hFile_cfg, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_cfg );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for reading.
	}

	if ( ret_status != 0 )
	{
		cfg_odd_row_font_settings.font = _CreateFontIndirectW( &g_default_log_font );
		//cfg_odd_row_font_settings.lf = g_default_log_font;
		_memcpy_s( &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ), &g_default_log_font, sizeof( LOGFONT ) );

		cfg_even_row_font_settings.font = _CreateFontIndirectW( &g_default_log_font );
		//cfg_even_row_font_settings.lf = g_default_log_font;
		_memcpy_s( &cfg_even_row_font_settings.lf, sizeof( LOGFONT ), &g_default_log_font, sizeof( LOGFONT ) );
	}
	else
	{
		cfg_odd_row_font_settings.font = _CreateFontIndirectW( &cfg_odd_row_font_settings.lf );

		cfg_even_row_font_settings.font = _CreateFontIndirectW( &cfg_even_row_font_settings.lf );
	}

	if ( cfg_default_download_directory == NULL )
	{
		cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );

		// Saves into C:\Users\[USER]\Downloads
		if ( shell32_state == SHELL32_STATE_RUNNING )
		{
			_SHGetKnownFolderPath = ( pSHGetKnownFolderPath )GetProcAddress( hModule_shell32, "SHGetKnownFolderPath" );
			if ( _SHGetKnownFolderPath != NULL )
			{
				bool free_memory = true;
				#ifndef OLE32_USE_STATIC_LIB
					if ( ole32_state == OLE32_STATE_SHUTDOWN )
					{
						free_memory = InitializeOle32();
					}
				#endif

				// Make sure we can call CoTaskMemFree.
				if ( free_memory )
				{
					wchar_t *folder_path = NULL;
					if ( _SHGetKnownFolderPath( ( REFKNOWNFOLDERID )FOLDERID_Downloads, 0, ( HANDLE )NULL, &folder_path ) == S_OK )
					{
						g_default_download_directory_length = min( MAX_PATH - 1, lstrlenW( folder_path ) );
						_wmemcpy_s( cfg_default_download_directory, MAX_PATH, folder_path, g_default_download_directory_length );
						cfg_default_download_directory[ g_default_download_directory_length ] = 0;
					}

					if ( folder_path != NULL )
					{
						_CoTaskMemFree( folder_path );
					}
				}
			}
		}

		// Saves into C:\Users\[USER]\Documents\Downloads
		if ( g_default_download_directory_length == 0 )
		{
			_SHGetFolderPathW( NULL, CSIDL_MYDOCUMENTS, NULL, 0, cfg_default_download_directory );

			g_default_download_directory_length = lstrlenW( cfg_default_download_directory );
			cfg_default_download_directory[ g_default_download_directory_length ] = L'\\';
			_wmemcpy_s( cfg_default_download_directory + ( g_default_download_directory_length + 1 ), MAX_PATH - ( g_default_download_directory_length + 1 ), ST_V_Downloads, ST_L_Downloads + 1 );
			cfg_default_download_directory[ MAX_PATH - 1 ] = 0;	// Sanity.
		}

		// Check to see if the new path exists and create it if it doesn't.
		if ( GetFileAttributesW( cfg_default_download_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			CreateDirectoryW( cfg_default_download_directory, NULL );
		}
	}

	if ( cfg_server_hostname == NULL )
	{
		cfg_server_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_server_hostname, 10, L"localhost\0", 10 );
		//cfg_server_hostname[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_hostname == NULL )
	{
		cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_hostname, 10, L"localhost\0", 10 );
		//cfg_hostname[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_hostname_s == NULL )
	{
		cfg_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_hostname_s, 10, L"localhost\0", 10 );
		//cfg_hostname_s[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_hostname_socks == NULL )
	{
		cfg_hostname_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_hostname_socks, 10, L"localhost\0", 10 );
		//cfg_hostname_socks[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_ftp_hostname == NULL )
	{
		cfg_ftp_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 10 );
		_wmemcpy_s( cfg_ftp_hostname, 10, L"localhost\0", 10 );
		//cfg_ftp_hostname[ 9 ] = 0;	// Sanity.
	}

	if ( cfg_temp_download_directory == NULL )
	{
		cfg_temp_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
		if ( cfg_temp_download_directory != NULL )
		{
			_wmemcpy_s( cfg_temp_download_directory, g_base_directory_length, g_base_directory, g_base_directory_length );
			_wmemcpy_s( cfg_temp_download_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\incomplete\0", 12 );
			g_temp_download_directory_length = g_base_directory_length + 11;
			//cfg_temp_download_directory[ g_temp_download_directory_length ] = 0;	// Sanity.

			// Check to see if the new path exists and create it if it doesn't.
			if ( GetFileAttributesW( cfg_temp_download_directory ) == INVALID_FILE_ATTRIBUTES )
			{
				CreateDirectoryW( cfg_temp_download_directory, NULL );
			}
		}
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading configuration: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_config()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\http_downloader_settings\0", 26 );
	//g_base_directory[ g_base_directory_length + 25 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving configuration: %S", g_base_directory );
#endif

	HANDLE hFile_cfg = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_cfg = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_cfg, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		int reserved = 1024 - 749;
		int size = ( sizeof( int ) * ( 11 + NUM_COLUMNS ) ) +
				   ( sizeof( unsigned short ) * 7 ) +
				   ( sizeof( char ) * ( 35 + NUM_COLUMNS + KEX_ALGORITHM_COUNT + HOST_KEY_COUNT + ENCRYPTION_CIPHER_COUNT ) ) +
				   ( sizeof( bool ) * 54 ) +
				   ( sizeof( unsigned long ) * 7 ) +
				   ( sizeof( LONG ) * 4 ) +
				   ( sizeof( BYTE ) * 6 ) +
				   ( sizeof( COLORREF ) * ( NUM_COLORS + 11 + TD_NUM_COLORS ) ) +
				   ( sizeof( unsigned long long ) * 5 ) + reserved;
		int pos = 0;
		unsigned char i;

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( write_buf + pos, size - pos, MAGIC_ID_SETTINGS, sizeof( char ) * 4 );	// Magic identifier for the main program's settings.
		pos += ( sizeof( char ) * 4 );

		// Main window.

		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_x, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_pos_y, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_width, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_height, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_min_max, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		for ( i = 0; i < NUM_COLUMNS; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, download_columns_width[ i ], sizeof( int ) );
			pos += sizeof( int );
		}

		for ( i = 0; i < NUM_COLUMNS; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, download_columns[ i ], sizeof( char ) );
			pos += sizeof( char );
		}

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_column_headers, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_sorted_column_index, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_sorted_direction, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_toolbar, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_status_bar, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_categories, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_splitter_pos_x, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_t_down_speed, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_t_downloaded, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_t_file_size, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_t_speed_limit, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_t_status_downloaded, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_t_status_down_speed, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_t_status_speed_limit, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		// Global Speed Limit

		_memcpy_s( write_buf + pos, size - pos, &cfg_download_speed_limit, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		//

		_memcpy_s( write_buf + pos, size - pos, &cfg_total_downloaded, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		// Options General

		_memcpy_s( write_buf + pos, size - pos, &cfg_tray_icon, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_minimize_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_close_to_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_start_in_tray, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_show_notification, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_show_tray_progress, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_always_on_top, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_check_for_updates, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_drop_window, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_drop_window_transparency, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_show_drop_window_progress, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_drop_pos_x, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_drop_pos_y, sizeof( int ) );
		pos += sizeof( int );

		_memcpy_s( write_buf + pos, size - pos, &cfg_play_sound, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_play_sound_fail, sizeof( bool ) );
		pos += sizeof( bool );

		// Options Appearance

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_gridlines, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_draw_full_rows, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_draw_all_rows, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_show_part_progress, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_sort_added_and_updating_items, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_expand_added_group_items, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_scroll_to_last_item, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_show_embedded_icon, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_background_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_gridline_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_selection_marquee_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );

		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.font_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.lf.lfHeight, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.lf.lfWeight, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.lf.lfItalic, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.lf.lfUnderline, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_font_settings.lf.lfStrikeOut, sizeof( BYTE ) );
		pos += sizeof( BYTE );

		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_background_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_highlight_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_odd_row_highlight_font_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );

		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.font_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.lf.lfHeight, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.lf.lfWeight, sizeof( LONG ) );
		pos += sizeof( LONG );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.lf.lfItalic, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.lf.lfUnderline, sizeof( BYTE ) );
		pos += sizeof( BYTE );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_font_settings.lf.lfStrikeOut, sizeof( BYTE ) );
		pos += sizeof( BYTE );

		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_background_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_highlight_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );
		_memcpy_s( write_buf + pos, size - pos, &cfg_even_row_highlight_font_color, sizeof( COLORREF ) );
		pos += sizeof( COLORREF );

		for ( i = 0; i < NUM_COLORS; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, progress_colors[ i ], sizeof( COLORREF ) );
			pos += sizeof( COLORREF );
		}

		// Tray and URL Drop window progress colors.

		for ( i = 0; i < TD_NUM_COLORS; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, td_progress_colors[ i ], sizeof( COLORREF ) );
			pos += sizeof( COLORREF );
		}

		// Options Connection

		_memcpy_s( write_buf + pos, size - pos, &cfg_max_downloads, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_default_download_parts, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_retry_downloads_count, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_retry_parts_count, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_timeout, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_max_redirects, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_default_speed_limit, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_default_ssl_version, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		// Options FTP

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_mode_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_enable_fallback_mode, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_address_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_ip_address, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_port_start, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_port_end, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ftp_send_keep_alive, sizeof( bool ) );
		pos += sizeof( bool );

		// Options Proxy

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_proxy, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_address_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ip_address, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_port, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		//

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_proxy_s, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_address_type_s, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ip_address_s, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_port_s, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		//

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_proxy_socks, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_socks_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_address_type_socks, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_ip_address_socks, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_port_socks, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_use_authentication_socks, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_resolve_domain_names_v4a, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_resolve_domain_names, sizeof( bool ) );
		pos += sizeof( bool );

		// Options Server

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_server, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_server_address_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_server_ip_address, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_server_port, sizeof( unsigned short ) );
		pos += sizeof( unsigned short );

		_memcpy_s( write_buf + pos, size - pos, &cfg_use_authentication, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_authentication_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_server_enable_ssl, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_certificate_type, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_server_ssl_version, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_show_remote_connection_notification, sizeof( bool ) );
		pos += sizeof( bool );

		// Options Advanced

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_download_history, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_quick_allocation, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_set_filetime, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_update_redirected, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_use_one_instance, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_prevent_standby, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_resume_downloads, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_drag_and_drop_action, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_prompt_last_modified, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_prompt_rename, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_prompt_file_size, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );
		_memcpy_s( write_buf + pos, size - pos, &cfg_max_file_size, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_shutdown_action, sizeof( unsigned char ) );
		pos += sizeof( unsigned char );

		_memcpy_s( write_buf + pos, size - pos, &cfg_use_temp_download_directory, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_thread_count, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		// Options SFTP

		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_enable_compression, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_attempt_gssapi_authentication, sizeof( bool ) );
		pos += sizeof( bool );
		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_attempt_gssapi_key_exchange, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_keep_alive_time, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_rekey_time, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_gss_rekey_time, sizeof( int ) );
		pos += sizeof( int );
		_memcpy_s( write_buf + pos, size - pos, &cfg_sftp_rekey_data_limit, sizeof( unsigned long ) );
		pos += sizeof( unsigned long );

		for ( i = 0; i < KEX_ALGORITHM_COUNT; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, &cfg_priority_kex_algorithm[ i ], sizeof( unsigned char ) );
			pos += sizeof( unsigned char );
		}

		for ( i = 0; i < HOST_KEY_COUNT; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, &cfg_priority_host_key[ i ], sizeof( unsigned char ) );
			pos += sizeof( unsigned char );
		}

		for ( i = 0; i < ENCRYPTION_CIPHER_COUNT; ++i )
		{
			_memcpy_s( write_buf + pos, size - pos, &cfg_priority_encryption_cipher[ i ], sizeof( unsigned char ) );
			pos += sizeof( unsigned char );
		}

		//

		_memcpy_s( write_buf + pos, size - pos, &cfg_reallocate_parts, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_reallocate_threshold_size, sizeof( unsigned long long ) );
		pos += sizeof( unsigned long long );

		_memcpy_s( write_buf + pos, size - pos, &cfg_download_non_200_206, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_move_to_trash, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_enable_sparse_file_allocation, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_override_list_prompts, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_apply_initial_proxy, sizeof( bool ) );
		pos += sizeof( bool );

		_memcpy_s( write_buf + pos, size - pos, &cfg_category_move, sizeof( bool ) );
		pos += sizeof( bool );

		//


		// Write Reserved bytes.
		_memzero( write_buf + pos, size - pos );

		DWORD write = 0;
		WriteFile( hFile_cfg, write_buf, size, &write, NULL );
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		GlobalFree( write_buf );

		int cfg_val_length = 0;
		char *utf8_cfg_val = NULL;


		//


		// Options General

		if ( cfg_sound_file_path != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_sound_file_path, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_sound_fail_file_path != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_sound_fail_file_path, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		// Options Appearance

		utf8_cfg_val = WideStringToUTF8String( cfg_odd_row_font_settings.lf.lfFaceName, &cfg_val_length );
		WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
		GlobalFree( utf8_cfg_val );
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		utf8_cfg_val = WideStringToUTF8String( cfg_even_row_font_settings.lf.lfFaceName, &cfg_val_length );
		WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
		GlobalFree( utf8_cfg_val );
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		// Options FTP

		if ( cfg_ftp_hostname != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_ftp_hostname, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		// Options Proxy

		if ( cfg_hostname != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_hostname, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_username != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_username, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_password != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_password, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		//

		if ( cfg_hostname_s != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_hostname_s, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_username_s != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_username_s, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_password_s != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_password_s, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		//

		if ( cfg_hostname_socks != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_hostname_socks, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_ident_username_socks != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_ident_username_socks, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_username_socks != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_username_socks, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_proxy_auth_password_socks != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_proxy_auth_password_socks, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		// Options Server

		if ( cfg_server_hostname != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_server_hostname, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_authentication_username != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_authentication_username, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_authentication_password != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_authentication_password, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_certificate_pkcs_password != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_certificate_pkcs_password, &cfg_val_length, sizeof( unsigned short ) );	// Add 2 bytes for our encoded length.

			int length = cfg_val_length - 1;	// Exclude the NULL terminator.
			_memcpy_s( utf8_cfg_val, cfg_val_length, &length, sizeof( unsigned short ) );

			encode_cipher( utf8_cfg_val + sizeof( unsigned short ), length );

			WriteFile( hFile_cfg, utf8_cfg_val, length + sizeof( unsigned short ), &write, NULL );	// Do not write the NULL terminator.
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0\0", 2, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_certificate_pkcs_file_name != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_certificate_pkcs_file_name, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_certificate_cer_file_name != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_certificate_cer_file_name, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_certificate_key_file_name != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_certificate_key_file_name, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		// Options Advanced

		if ( cfg_default_download_directory != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_default_download_directory, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		if ( cfg_temp_download_directory != NULL )
		{
			utf8_cfg_val = WideStringToUTF8String( cfg_temp_download_directory, &cfg_val_length );
			WriteFile( hFile_cfg, utf8_cfg_val, cfg_val_length, &write, NULL );
			GlobalFree( utf8_cfg_val );
		}
		else
		{
			WriteFile( hFile_cfg, "\0", 1, &write, NULL );
		}
#ifdef ENABLE_LOGGING
		lfz += write;
#endif

		SetEndOfFile( hFile_cfg );

		UnlockFileEx( hFile_cfg, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_cfg );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving configuration: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char read_download_history( wchar_t *file_path, bool scroll_to_last_item )
{
	char ret_status = 0;
	char open_count = 0;

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading download history: %S", file_path );
#endif

	HANDLE hFile_read = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_read = CreateFile( file_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_read, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		TREELISTNODE		*tln_parent = NULL;
		DOWNLOAD_INFO		*shared_info = NULL;

		ULARGE_INTEGER		add_time;
		unsigned long long	downloaded;
		unsigned long long	file_size;
		unsigned long long	download_speed_limit;

		char				*category = NULL;
		unsigned int		category_length;
		char				*download_directory = NULL;
		unsigned int		download_directory_length;
		char				*filename = NULL;
		unsigned int		filename_length;

		wchar_t				*url = NULL;
		DoublyLinkedList	*range_list = NULL;
		unsigned char		parts;
		unsigned char		parts_limit;
		unsigned int		status;
		int					code;

		wchar_t				*comments = NULL;
		unsigned int		comments_length;

		char				*cookies = NULL;
		char				*headers = NULL;
		char				*data = NULL;

		char				*username = NULL;
		char				*password = NULL;

		char				ssl_version;

		bool				processed_header;
		unsigned int		download_operations;
		unsigned char		method;

		ULARGE_INTEGER		last_modified;

		unsigned char		range_count;

		//

		unsigned char		proxy_type;

		unsigned char		proxy_address_type;

		unsigned int		proxy_hostname_length;
		wchar_t				*proxy_hostname = NULL;
		unsigned long		proxy_ip_address;

		unsigned short		proxy_port;

		char				*proxy_auth_username = NULL;
		char				*proxy_auth_password = NULL;
		unsigned int		proxy_auth_username_length;
		unsigned int		proxy_auth_password_length;
		wchar_t				*w_proxy_auth_username = NULL;
		wchar_t				*w_proxy_auth_password = NULL;

		bool				proxy_resolve_domain_names;
		bool				proxy_use_authentication;

		//
		int					total_item_count = 0;
		int					expanded_item_count = 0;
		int					root_item_count = 0;
		//

		unsigned char magic_identifier[ 4 ];
		BOOL bRet = ReadFile( hFile_read, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
		if ( bRet != FALSE )
		{
#ifdef ENABLE_LOGGING
			lfz += 4;
#endif
			unsigned char version = magic_identifier[ 3 ] - 0x10;

			if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_DOWNLOADS, 3 ) == 0 && version <= 0x0F )
			{
				char *history_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 3 ) );	// 512 KB buffer.
				if ( history_buf != NULL )
				{
					DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

					while ( total_read < fz )
					{
						bRet = ReadFile( hFile_read, history_buf, sizeof( char ) * 524288, &read, NULL );
						if ( bRet == FALSE )
						{
							break;
						}

#ifdef ENABLE_LOGGING
						lfz += read;
#endif

						history_buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

						// This terminates wide character strings so we don't read past the buffer.
						history_buf[ read + 1 ] = 0;
						history_buf[ read + 2 ] = 0;

						total_read += read;

						// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
						// If it's larger than our buffer, then the file is probably invalid/corrupt.
						if ( total_read == last_total )
						{
							break;
						}

						last_total = total_read;

						p = history_buf;
						offset = last_entry = 0;

						while ( offset < read )
						{
							tln_parent = NULL;
							shared_info = NULL;

							//

							url = NULL;
							comments = NULL;
							cookies = NULL;
							headers = NULL;
							data = NULL;
							username = NULL;
							password = NULL;
							range_list = NULL;

							//

							proxy_hostname = NULL;

							proxy_auth_username = NULL;
							proxy_auth_password = NULL;
							w_proxy_auth_username = NULL;
							w_proxy_auth_password = NULL;

							//

							category = NULL;
							category_length = 0;
							download_directory = NULL;
							download_directory_length = 0;
							filename = NULL;
							filename_length = 0;

							//

							unsigned char host_count = 0;
							bool is_expanded = false;

							// Host Count
							offset += sizeof( unsigned char );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &host_count, sizeof( unsigned char ), p, sizeof( unsigned char ) );
							p += sizeof( unsigned char );
							if ( host_count == 0 ) { goto CLEANUP; }

							// Item Expansion
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &is_expanded, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							// Add Time
							offset += sizeof( ULONGLONG );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &add_time.QuadPart, sizeof( ULONGLONG ), p, sizeof( ULONGLONG ) );
							p += sizeof( ULONGLONG );

							// Downloaded
							offset += sizeof( unsigned long long );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &downloaded, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
							p += sizeof( unsigned long long );

							// File Size
							offset += sizeof( unsigned long long );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &file_size, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
							p += sizeof( unsigned long long );

							// Download Speed Limit
							offset += sizeof( unsigned long long );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &download_speed_limit, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
							p += sizeof( unsigned long long );

							// Create Hosts
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &processed_header, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							// Download Operations
							if ( version >= 0x07 )
							{
								offset += sizeof( unsigned int );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &download_operations, sizeof( unsigned int ), p, sizeof( unsigned int ) );
								p += sizeof( unsigned int );
							}
							else
							{
								download_operations = 0;

								offset += sizeof( unsigned char );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &download_operations, sizeof( unsigned int ), p, sizeof( unsigned char ) );
								p += sizeof( unsigned char );
							}

							int string_length;

							// Category
							if ( version >= 0x08 )
							{
								string_length = lstrlenW( ( wchar_t * )p ) + 1;

								offset += ( string_length * sizeof( wchar_t ) );
								if ( offset >= read ) { goto CLEANUP; }

								category = p;
								category_length = string_length;

								p += ( string_length * sizeof( wchar_t ) );
							}

							// Download Directory
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							download_directory = p;
							download_directory_length = string_length;

							p += ( string_length * sizeof( wchar_t ) );

							// Filename
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							filename = p;
							filename_length = string_length - 1;

							p += ( string_length * sizeof( wchar_t ) );

							// Comments
							if ( version >= 0x08 )
							{
								string_length = lstrlenW( ( wchar_t * )p ) + 1;

								offset += ( string_length * sizeof( wchar_t ) );
								if ( offset >= read ) { goto CLEANUP; }

								comments = ( wchar_t * )p;
								comments_length = string_length;

								p += ( string_length * sizeof( wchar_t ) );
							}
							else
							{
								comments_length = 0;
							}

							////////////////

							shared_info = ( DOWNLOAD_INFO * )GlobalAlloc( GPTR, sizeof( DOWNLOAD_INFO ) );
							if ( shared_info != NULL )
							{
								shared_info->hFile = INVALID_HANDLE_VALUE;

								shared_info->add_time.QuadPart = add_time.QuadPart;
								shared_info->downloaded = downloaded;
								shared_info->last_downloaded = downloaded;
								shared_info->file_size = file_size;
								shared_info->download_speed_limit = download_speed_limit;
								shared_info->processed_header = processed_header;
								shared_info->download_operations = download_operations;

								if ( category_length > 1 )	// Don't cache the empty string.
								{
									shared_info->category = CacheCategory( ( wchar_t * )category );
								}

								_wmemcpy_s( shared_info->file_path, MAX_PATH, download_directory, download_directory_length );
								shared_info->file_path[ download_directory_length ] = 0;	// Sanity.

								shared_info->filename_offset = download_directory_length;	// Includes the NULL terminator.

								_wmemcpy_s( shared_info->file_path + shared_info->filename_offset, MAX_PATH - shared_info->filename_offset, filename, filename_length + 1 );
								shared_info->file_path[ shared_info->filename_offset + filename_length + 1 ] = 0;	// Sanity.

								shared_info->file_extension_offset = shared_info->filename_offset + ( ( shared_info->download_operations & DOWNLOAD_OPERATION_GET_EXTENSION ) ? filename_length : get_file_extension_offset( shared_info->file_path + shared_info->filename_offset, filename_length ) );

								// Couldn't get an extension, try to get one from the Content-Type header field.
								if ( shared_info->file_extension_offset == ( shared_info->filename_offset + filename_length ) )
								{
									shared_info->download_operations |= DOWNLOAD_OPERATION_GET_EXTENSION;
								}

								// Don't use empty string.
								if ( comments_length > 1 )
								{
									shared_info->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * comments_length );
									_wmemcpy_s( shared_info->comments, comments_length, comments, comments_length );
								}

								//

								SYSTEMTIME st;
								FILETIME ft;
								ft.dwHighDateTime = shared_info->add_time.HighPart;
								ft.dwLowDateTime = shared_info->add_time.LowPart;
								FileTimeToSystemTime( &ft, &st );

								int buffer_length = 0;

								#ifndef NTDLL_USE_STATIC_LIB
									//buffer_length = 64;	// Should be enough to hold most translated values.
									buffer_length = __snwprintf( NULL, 0, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
								#else
									buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
								#endif

								shared_info->w_add_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

								__snwprintf( shared_info->w_add_time, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );

								//

								shared_info->ssl_version = -1;

								////////////////

								shared_info->shared_info = shared_info;

								InitializeCriticalSection( &shared_info->di_cs );

								tln_parent = ( TREELISTNODE * )GlobalAlloc( GPTR, sizeof( TREELISTNODE ) );
								shared_info->tln = ( void * )tln_parent;
								tln_parent->data = shared_info;
								tln_parent->data_type = TLVDT_GROUP | ( host_count == 1 ? TLVDT_HOST : 0 );
								tln_parent->is_expanded = is_expanded;

								TLV_AddNode( &g_tree_list, tln_parent, -1 );

								++root_item_count;
								++expanded_item_count;
								++total_item_count;

								////////////////

								for ( unsigned char i = 0; i < host_count; ++i )
								{
									url = NULL;
									comments = NULL;
									cookies = NULL;
									headers = NULL;
									data = NULL;
									username = NULL;
									password = NULL;
									range_list = NULL;

									code = 0;

									//

									proxy_type = 0;
									proxy_address_type = 0;
									proxy_ip_address = 0;
									proxy_port = 0;

									proxy_hostname = NULL;
									proxy_hostname_length = 0;

									proxy_auth_username = NULL;
									proxy_auth_password = NULL;
									proxy_auth_username_length = 0;
									proxy_auth_password_length = 0;
									w_proxy_auth_username = NULL;
									w_proxy_auth_password = NULL;

									proxy_resolve_domain_names = false;
									proxy_use_authentication = false;

									//

									if ( host_count > 1 )
									{
										// Downloaded
										offset += sizeof( unsigned long long );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &downloaded, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
										p += sizeof( unsigned long long );

										// File Size
										offset += sizeof( unsigned long long );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &file_size, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
										p += sizeof( unsigned long long );

										// Download Speed Limit
										offset += sizeof( unsigned long long );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &download_speed_limit, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
										p += sizeof( unsigned long long );

										// Create Range
										offset += sizeof( bool );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &processed_header, sizeof( bool ), p, sizeof( bool ) );
										p += sizeof( bool );
									}

									// Parts
									offset += sizeof( unsigned char );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &parts, sizeof( unsigned char ), p, sizeof( unsigned char ) );
									p += sizeof( unsigned char );

									// Parts Limit
									offset += sizeof( unsigned char );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &parts_limit, sizeof( unsigned char ), p, sizeof( unsigned char ) );
									p += sizeof( unsigned char );

									// Status
									offset += sizeof( unsigned int );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &status, sizeof( unsigned int ), p, sizeof( unsigned int ) );
									p += sizeof( unsigned int );

									if ( version >= 0x06 )
									{
										// Code
										offset += sizeof( int );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &code, sizeof( int ), p, sizeof( int ) );
										p += sizeof( int );
									}

									// SSL Version
									offset += sizeof( char );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &ssl_version, sizeof( char ), p, sizeof( char ) );
									p += sizeof( char );

									// Method
									offset += sizeof( unsigned char );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &method, sizeof( unsigned char ), p, sizeof( unsigned char ) );
									p += sizeof( unsigned char );

									// Last Modified
									offset += sizeof( ULONGLONG );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &last_modified.QuadPart, sizeof( ULONGLONG ), p, sizeof( ULONGLONG ) );
									p += sizeof( ULONGLONG );

									// URL
									string_length = lstrlenW( ( wchar_t * )p ) + 1;

									offset += ( string_length * sizeof( wchar_t ) );
									if ( offset >= read ) { goto CLEANUP; }

									url = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
									_wmemcpy_s( url, string_length, p, string_length );
									*( url + ( string_length - 1 ) ) = 0;	// Sanity

									p += ( string_length * sizeof( wchar_t ) );

									// Comments
									if ( version >= 0x08 )
									{
										if ( host_count > 1 )
										{
											string_length = lstrlenW( ( wchar_t * )p ) + 1;

											offset += ( string_length * sizeof( wchar_t ) );
											if ( offset >= read ) { goto CLEANUP; }

											comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
											_wmemcpy_s( comments, string_length, p, string_length );
											*( comments + ( string_length - 1 ) ) = 0;	// Sanity

											p += ( string_length * sizeof( wchar_t ) );
										}
										else
										{
											offset += sizeof( wchar_t );
											if ( offset >= read ) { goto CLEANUP; }

											p += sizeof( wchar_t );
										}
									}

									// Cookies
									string_length = lstrlenA( ( char * )p ) + 1;

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }

									// Let's not allocate an empty string.
									if ( string_length > 1 )
									{
										cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
										_memcpy_s( cookies, string_length, p, string_length );
										*( cookies + ( string_length - 1 ) ) = 0;	// Sanity
									}

									p += string_length;

									// Headers
									string_length = lstrlenA( ( char * )p ) + 1;

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }

									// Let's not allocate an empty string.
									if ( string_length > 1 )
									{
										headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
										_memcpy_s( headers, string_length, p, string_length );
										*( headers + ( string_length - 1 ) ) = 0;	// Sanity
									}

									p += string_length;

									// Data
									string_length = lstrlenA( ( char * )p ) + 1;

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }

									// Let's not allocate an empty string.
									if ( string_length > 1 )
									{
										data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
										_memcpy_s( data, string_length, p, string_length );
										*( data + ( string_length - 1 ) ) = 0;	// Sanity
									}

									p += string_length;

									// Username
									offset += sizeof( int );
									if ( offset >= read ) { goto CLEANUP; }

									// Length of the string - not including the NULL character.
									_memcpy_s( &string_length, sizeof( int ), p, sizeof( int ) );
									p += sizeof( int );

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }
									if ( string_length > 0 )
									{
										// string_length does not contain the NULL character of the string.
										username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( username, string_length, p, string_length );
										username[ string_length ] = 0; // Sanity;

										decode_cipher( username, string_length );

										p += string_length;
									}

									// Password
									offset += sizeof( int );
									if ( offset >= read ) { goto CLEANUP; }

									// Length of the string - not including the NULL character.
									_memcpy_s( &string_length, sizeof( int ), p, sizeof( int ) );
									p += sizeof( int );

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }
									if ( string_length > 0 )
									{
										// string_length does not contain the NULL character of the string.
										password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( password, string_length, p, string_length );
										password[ string_length ] = 0; // Sanity;

										decode_cipher( password, string_length );

										p += string_length;
									}

									// Proxy Info

									// Proxy Type
									offset += sizeof( unsigned char );
									if ( offset > read ) { goto CLEANUP; }
									_memcpy_s( &proxy_type, sizeof( unsigned char ), p, sizeof( unsigned char ) );
									p += sizeof( unsigned char );

									if ( proxy_type != 0 )
									{
										// Proxy Address Type
										offset += sizeof( unsigned char );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &proxy_address_type, sizeof( unsigned char ), p, sizeof( unsigned char ) );
										p += sizeof( unsigned char );

										if ( proxy_address_type == 0 )
										{
											// Proxy Hostname
											proxy_hostname_length = lstrlenW( ( wchar_t * )p );

											string_length = proxy_hostname_length + 1;

											offset += ( string_length * sizeof( wchar_t ) );
											if ( offset >= read ) { goto CLEANUP; }

											proxy_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
											_wmemcpy_s( proxy_hostname, string_length, p, string_length );
											*( proxy_hostname + ( string_length - 1 ) ) = 0;	// Sanity

											p += ( string_length * sizeof( wchar_t ) );
										}
										else// if ( proxy_address_type == 1 )
										{
											// Proxy IP Address
											offset += sizeof( unsigned long );
											if ( offset >= read ) { goto CLEANUP; }
											_memcpy_s( &proxy_ip_address, sizeof( unsigned long ), p, sizeof( unsigned long ) );
											p += sizeof( unsigned long );
										}

										// Proxy Port
										offset += sizeof( unsigned short );
										if ( offset >= read ) { goto CLEANUP; }
										_memcpy_s( &proxy_port, sizeof( unsigned short ), p, sizeof( unsigned short ) );
										p += sizeof( unsigned short );

										if ( proxy_type == 1 || proxy_type == 2 )	// HTTP and HTTPS
										{
											// Proxy Username
											offset += sizeof( unsigned short );
											if ( offset >= read ) { goto CLEANUP; }

											// Length of the string - not including the NULL character.
											_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
											p += sizeof( unsigned short );

											offset += string_length;
											if ( offset >= read ) { goto CLEANUP; }
											if ( string_length > 0 )
											{
												proxy_auth_username_length = string_length;

												// string_length does not contain the NULL character of the string.
												proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
												_memcpy_s( proxy_auth_username, string_length, p, string_length );
												proxy_auth_username[ string_length ] = 0; // Sanity;

												decode_cipher( proxy_auth_username, string_length );

												w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

												p += string_length;
											}

											// Proxy Password
											offset += sizeof( unsigned short );
											if ( offset > read ) { goto CLEANUP; }

											// Length of the string - not including the NULL character.
											_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
											p += sizeof( unsigned short );

											offset += string_length;
											if ( offset > read ) { goto CLEANUP; }
											if ( string_length > 0 )
											{
												proxy_auth_password_length = string_length;

												// string_length does not contain the NULL character of the string.
												proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
												_memcpy_s( proxy_auth_password, string_length, p, string_length );
												proxy_auth_password[ string_length ] = 0; // Sanity;

												decode_cipher( proxy_auth_password, string_length );

												w_proxy_auth_password = UTF8StringToWideString( proxy_auth_password, string_length + 1 );

												p += string_length;
											}
										}
										else if ( proxy_type == 3 )	// SOCKS v4
										{
											// Resolve Domain Names
											offset += sizeof( bool );
											if ( offset >= read ) { goto CLEANUP; }
											_memcpy_s( &proxy_resolve_domain_names, sizeof( bool ), p, sizeof( bool ) );
											p += sizeof( bool );

											// Proxy Username
											offset += sizeof( unsigned short );
											if ( offset > read ) { goto CLEANUP; }

											// Length of the string - not including the NULL character.
											_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
											p += sizeof( unsigned short );

											offset += string_length;
											if ( offset > read ) { goto CLEANUP; }
											if ( string_length > 0 )
											{
												// string_length does not contain the NULL character of the string.
												proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
												_memcpy_s( proxy_auth_username, string_length, p, string_length );
												proxy_auth_username[ string_length ] = 0; // Sanity;

												decode_cipher( proxy_auth_username, string_length );

												w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

												p += string_length;
											}
										}
										else if ( proxy_type == 4 )	// SOCKS v5
										{
											// Resolve Domain Names
											offset += sizeof( bool );
											if ( offset >= read ) { goto CLEANUP; }
											_memcpy_s( &proxy_resolve_domain_names, sizeof( bool ), p, sizeof( bool ) );
											p += sizeof( bool );

											// Use Authentication
											offset += sizeof( bool );
											if ( offset >= read ) { goto CLEANUP; }
											_memcpy_s( &proxy_use_authentication, sizeof( bool ), p, sizeof( bool ) );
											p += sizeof( bool );

											if ( proxy_use_authentication )
											{
												// Proxy Username
												offset += sizeof( unsigned short );
												if ( offset >= read ) { goto CLEANUP; }

												// Length of the string - not including the NULL character.
												_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
												p += sizeof( unsigned short );

												offset += string_length;
												if ( offset >= read ) { goto CLEANUP; }
												if ( string_length > 0 )
												{
													// string_length does not contain the NULL character of the string.
													proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
													_memcpy_s( proxy_auth_username, string_length, p, string_length );
													proxy_auth_username[ string_length ] = 0; // Sanity;

													decode_cipher( proxy_auth_username, string_length );

													w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

													p += string_length;
												}

												// Proxy Password
												offset += sizeof( unsigned short );
												if ( offset > read ) { goto CLEANUP; }

												// Length of the string - not including the NULL character.
												_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
												p += sizeof( unsigned short );

												offset += string_length;
												if ( offset > read ) { goto CLEANUP; }
												if ( string_length > 0 )
												{
													// string_length does not contain the NULL character of the string.
													proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
													_memcpy_s( proxy_auth_password, string_length, p, string_length );
													proxy_auth_password[ string_length ] = 0; // Sanity;

													decode_cipher( proxy_auth_password, string_length );

													w_proxy_auth_password = UTF8StringToWideString( proxy_auth_password, string_length + 1 );

													p += string_length;
												}
											}
										}
									}

									// Range Info.
									offset += sizeof( unsigned char );
									if ( offset <= read )
									{
										range_count = *p;
										p += sizeof( unsigned char );

										for ( unsigned char j = 0; j < range_count; ++j )
										{
											offset += ( sizeof( unsigned long long ) * 5 );
											if ( offset > read ) { goto CLEANUP; }

											RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );

											_memcpy_s( &ri->range_start, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
											p += sizeof( unsigned long long );

											_memcpy_s( &ri->range_end, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
											p += sizeof( unsigned long long );

											_memcpy_s( &ri->content_length, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
											p += sizeof( unsigned long long );

											_memcpy_s( &ri->content_offset, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
											p += sizeof( unsigned long long );

											_memcpy_s( &ri->file_write_offset, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
											p += sizeof( unsigned long long );

											DoublyLinkedList *range_node = DLL_CreateNode( ( void * )ri );
											DLL_AddNode( &range_list, range_node, -1 );
										}
									}

									DOWNLOAD_INFO *di = ( host_count == 1 ? shared_info : ( DOWNLOAD_INFO * )GlobalAlloc( GPTR, sizeof( DOWNLOAD_INFO ) ) );
									if ( di != NULL )
									{
										di->shared_info = shared_info;

										++di->shared_info->hosts;

										di->shared_info_node.data = di;
										DLL_AddNode( &di->shared_info->host_list, &di->shared_info_node, -1 );

										if ( host_count != 1 )
										{
											di->shared_info->parts += parts;

											InitializeCriticalSection( &di->di_cs );

											++tln_parent->child_count;

											TREELISTNODE *tln = ( TREELISTNODE * )GlobalAlloc( GPTR, sizeof( TREELISTNODE ) );
											tln->data = di;
											tln->data_type = TLVDT_HOST;
											tln->parent = tln_parent;

											TLV_AddNode( &tln_parent->child, tln, -1, true );

											if ( tln_parent->is_expanded )
											{
												++expanded_item_count;
											}

											++total_item_count;
										}

										di->downloaded = downloaded;
										di->last_downloaded = downloaded;
										di->file_size = file_size;
										di->download_speed_limit = download_speed_limit;
										di->parts = parts;
										di->parts_limit = parts_limit;
										SetStatus( di, status );
										di->code = code;
										if ( g_can_use_tls_1_3 && ssl_version >= 5 ) { ssl_version = 5; }	// TLS 1.3.
										else if ( ssl_version > 4 ) { ssl_version = 4; }	// TLS 1.2.
										di->ssl_version = ssl_version;
										di->processed_header = processed_header;
										di->method = method;
										di->last_modified.QuadPart = last_modified.QuadPart;
										di->url = url;
										if ( comments != NULL )
										{
											di->comments = comments;
										}
										di->cookies = cookies;
										di->headers = headers;
										di->data = data;
										di->auth_info.username = username;
										di->auth_info.password = password;

										di->range_list = range_list;
										di->print_range_list = di->range_list;

										//

										if ( proxy_type != 0 )
										{
											PROXY_INFO *pi = ( PROXY_INFO * )GlobalAlloc( GPTR, sizeof( PROXY_INFO ) );
											pi->type = proxy_type;
											pi->address_type = proxy_address_type;

											pi->hostname = proxy_hostname;
											pi->ip_address = proxy_ip_address;

											pi->port = proxy_port;

											pi->resolve_domain_names = proxy_resolve_domain_names;
											pi->use_authentication = proxy_use_authentication;

											pi->username = proxy_auth_username;
											pi->password = proxy_auth_password;
											pi->w_username = w_proxy_auth_username;
											pi->w_password = w_proxy_auth_password;

											if ( ( pi->type == 1 || pi->type == 2 ) && ( pi->username != NULL && pi->password != NULL ) )
											{
												CreateBasicAuthorizationKey( pi->username, proxy_auth_username_length, pi->password, proxy_auth_password_length, &pi->auth_key, &pi->auth_key_length );
											}

											//

											if ( pi->hostname != NULL )
											{
												if ( normaliz_state == NORMALIZ_STATE_RUNNING )
												{
													int punycode_length = _IdnToAscii( 0, pi->hostname, proxy_hostname_length, NULL, 0 );

													if ( punycode_length > ( int )proxy_hostname_length )
													{
														pi->punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
														_IdnToAscii( 0, pi->hostname, proxy_hostname_length, pi->punycode_hostname, punycode_length );
													}
													else
													{
														pi->punycode_hostname = NULL;
													}
												}
											}

											di->proxy_info = di->saved_proxy_info = pi;
										}

										//

										if ( di->status == STATUS_NONE ||
											 IS_STATUS( di->status, STATUS_PAUSED ) )	// No status set, or Paused
										{
											SetStatus( di, STATUS_STOPPED );	// Stopped
										}
										else if ( IS_STATUS( di->status,
													 STATUS_CONNECTING |
													 STATUS_DOWNLOADING |
													 STATUS_RESTART ) )	// Connecting, Downloading, Queued, or Restarting
										{
											if ( cfg_resume_downloads )
											{
												g_download_history_changed = true;

												SetStatus( di, STATUS_NONE );

												StartDownload( di, START_TYPE_NONE, START_OPERATION_NONE );
											}
											else
											{
												SetStatus( di, STATUS_STOPPED );	// Stopped
											}
										}
										else if ( di->status == STATUS_ALLOCATING_FILE )	// If we were allocating the file, then set it to a File IO Error.
										{
											SetStatus( di, STATUS_FILE_IO_ERROR );
										}
									}
									else
									{
										goto CLEANUP;
									}
								}
								
								last_entry = offset;	// This value is the ending offset of the last valid entry.

								// print_range_list is used in DrawTreeListView
								if ( host_count > 1 )
								{
									shared_info->print_range_list = shared_info->host_list;
								}

								SetSharedInfoStatus( shared_info );

								//

								// Cache our file's icon.
								ICON_INFO *ii = CacheIcon( shared_info );

								if ( ii != NULL )
								{
									shared_info->icon = &ii->icon;
								}

								continue;
							}

			CLEANUP:
							GlobalFree( url );
							GlobalFree( comments );
							GlobalFree( cookies );
							GlobalFree( headers );
							GlobalFree( data );
							GlobalFree( username );
							GlobalFree( password );

							//

							GlobalFree( proxy_hostname );

							GlobalFree( proxy_auth_username );
							GlobalFree( proxy_auth_password );
							GlobalFree( w_proxy_auth_username );
							GlobalFree( w_proxy_auth_password );

							//

							DoublyLinkedList *range_node;
							while ( range_list != NULL )
							{
								range_node = range_list;
								range_list = range_list->next;

								GlobalFree( range_node->data );
								GlobalFree( range_node );
							}

							//

							if ( shared_info != NULL )
							{
								while ( shared_info->host_list != NULL )
								{
									DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )shared_info->host_list->data;

									shared_info->host_list = shared_info->host_list->next;

									//

									if ( di != NULL )
									{
										GlobalFree( di->url );
										GlobalFree( di->comments );
										GlobalFree( di->cookies );
										GlobalFree( di->headers );
										GlobalFree( di->data );
										//GlobalFree( di->etag );
										GlobalFree( di->auth_info.username );
										GlobalFree( di->auth_info.password );

										// saved_proxy_info equals proxy_info here.
										/*if ( di->proxy_info != di->saved_proxy_info )
										{
											FreeProxyInfo( &di->saved_proxy_info );
										}*/
										FreeProxyInfo( &di->proxy_info );

										while ( di->range_list != NULL )
										{
											range_node = di->range_list;
											di->range_list = di->range_list->next;

											GlobalFree( range_node->data );
											GlobalFree( range_node );
										}

										DeleteCriticalSection( &di->di_cs );

										GlobalFree( di );
									}
								}

								DeleteCriticalSection( &shared_info->di_cs );

								RemoveCachedIcon( shared_info );
								RemoveCachedCategory( shared_info->category );
								GlobalFree( shared_info->comments );
								//GlobalFree( shared_info->new_file_path );	// Not set here.
								GlobalFree( shared_info->w_add_time );

								GlobalFree( shared_info );
							}

							//

							if ( tln_parent != NULL )
							{
								TREELISTNODE *tln_child = tln_parent->child;

								while ( tln_child != NULL )
								{
									TREELISTNODE *del_tln = tln_child;

									tln_child = tln_child->next;

									GlobalFree( del_tln );

									if ( tln_parent->is_expanded )
									{
										--expanded_item_count;
									}

									--total_item_count;
								}

								TLV_RemoveNode( &g_tree_list, tln_parent );

								GlobalFree( tln_parent );

								--root_item_count;
								--expanded_item_count;

								--total_item_count;
							}

							//

							// Go back to the last valid entry.
							if ( total_read < fz )
							{
								total_read -= ( read - last_entry );
								SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
							}

							break;
						}
					}

					GlobalFree( history_buf );
				}

				//

				if ( g_status_filter != STATUS_NONE )
				{
					total_item_count = root_item_count = expanded_item_count = 0;

					TREELISTNODE *node = g_tree_list;
					while ( node != NULL )
					{
						DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )node->data;
						if ( IsFilterSet( di, g_status_filter ) )
						{
							total_item_count += ( node->child_count + 1 );
							++root_item_count;
							++expanded_item_count;	// Include the parent.
							if ( node->is_expanded )
							{
								expanded_item_count += node->child_count;
							}
						}

						node = node->next;
					}

					TLV_SetTotalItemCount( total_item_count );
					TLV_SetExpandedItemCount( expanded_item_count );
					TLV_SetRootItemCount( root_item_count );
				}
				else
				{
					TLV_AddTotalItemCount( total_item_count );
					TLV_AddExpandedItemCount( expanded_item_count );
					TLV_AddRootItemCount( root_item_count );
					if ( TLV_GetFirstVisibleItem() == NULL )
					{
						TLV_SetFirstVisibleRootIndex( 0 );
						TLV_SetFirstVisibleItem( g_tree_list );
						TLV_SetFirstVisibleIndex( 0 );
					}
				}

				if ( cfg_sorted_column_index != COLUMN_NUM )		// #
				{
					SORT_INFO si;
					si.column = cfg_sorted_column_index;
					si.hWnd = g_hWnd_tlv_files;
					si.direction = cfg_sorted_direction;

					_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
				}

				_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, ( scroll_to_last_item ? TRUE : FALSE ) );
				if ( scroll_to_last_item )
				{
					UpdateMenus( true );
				}
				UpdateSBItemCount();
			}
			else
			{
				ret_status = -2;	// Bad file format.
			}
		}
		else
		{
			ret_status = -1;	// Can't open file for reading.
		}

		UnlockFileEx( hFile_read, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_read );	
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for reading.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading download history: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_download_history( wchar_t *file_path )
{
	char ret_status = 0;
	char open_count = 0;

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving download history: %S", file_path );
#endif

	HANDLE hFile_downloads = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_downloads = CreateFile( file_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_downloads != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_downloads, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_DOWNLOADS, sizeof( char ) * 4 );	// Magic identifier for the download history.
		pos += ( sizeof( char ) * 4 );

		TREELISTNODE *tln = g_tree_list;
		while ( tln != NULL )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
				// lstrlen is safe for NULL values.
				int category_length = ( lstrlenW( di->category ) + 1 ) * sizeof( wchar_t );
				int download_directory_length = di->filename_offset * sizeof( wchar_t );	// Includes the NULL terminator.
				int filename_length = ( lstrlenW( di->file_path + di->filename_offset ) + 1 ) * sizeof( wchar_t );
				int comments_length =  ( lstrlenW( di->comments ) + 1 ) * sizeof( wchar_t );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos +
								 comments_length +
								 filename_length +
								 download_directory_length +
								 category_length +
								 sizeof( ULONGLONG ) +
							   ( sizeof( unsigned long long ) * 3 ) +
							   ( sizeof( bool ) * 2 ) +
								 sizeof( unsigned int ) +
								 sizeof( unsigned char ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile_downloads, buf, pos, &write, NULL );
					pos = 0;
#ifdef ENABLE_LOGGING
					lfz += write;
#endif
				}

				_memcpy_s( buf + pos, size - pos, &di->hosts, sizeof( unsigned char ) );
				pos += sizeof( unsigned char );

				_memcpy_s( buf + pos, size - pos, &tln->is_expanded, sizeof( bool ) );
				pos += sizeof( bool );

				_memcpy_s( buf + pos, size - pos, &di->add_time.QuadPart, sizeof( ULONGLONG ) );
				pos += sizeof( ULONGLONG );

				_memcpy_s( buf + pos, size - pos, &di->downloaded, sizeof( unsigned long long ) );
				pos += sizeof( unsigned long long );

				_memcpy_s( buf + pos, size - pos, &di->file_size, sizeof( unsigned long long ) );
				pos += sizeof( unsigned long long );

				_memcpy_s( buf + pos, size - pos, &di->download_speed_limit, sizeof( unsigned long long ) );
				pos += sizeof( unsigned long long );

				_memcpy_s( buf + pos, size - pos, &di->processed_header, sizeof( bool ) );
				pos += sizeof( bool );

				_memcpy_s( buf + pos, size - pos, &di->download_operations, sizeof( unsigned int ) );
				pos += sizeof( unsigned int );

				_memcpy_s( buf + pos, size - pos, di->category, category_length );
				pos += category_length;

				_memcpy_s( buf + pos, size - pos, di->file_path, download_directory_length );
				pos += download_directory_length;

				_memcpy_s( buf + pos, size - pos, di->file_path + di->filename_offset, filename_length );
				pos += filename_length;

				_memcpy_s( buf + pos, size - pos, di->shared_info->comments, comments_length );
				pos += comments_length;

				///////////////////

				DoublyLinkedList *host_node = di->shared_info->host_list;

				while ( host_node != NULL )
				{
					di = ( DOWNLOAD_INFO * )host_node->data;

					int url_length = ( lstrlenW( di->url ) + 1 ) * sizeof( wchar_t );

					comments_length = ( lstrlenW( di->comments ) + 1 ) * sizeof( wchar_t );

					int cookies_length = lstrlenA( di->cookies ) + 1;
					int headers_length = lstrlenA( di->headers ) + 1;
					int data_length = lstrlenA( di->data ) + 1;

					int username_length = lstrlenA( di->auth_info.username );
					int password_length = lstrlenA( di->auth_info.password );

					int optional_extra_length = 0;

					int proxy_username_length = 0;
					int proxy_password_length = 0;
					int proxy_address_length = 0;

					PROXY_INFO *pi = di->saved_proxy_info;
					if ( pi != NULL && pi->type != 0 )
					{
						optional_extra_length += sizeof( unsigned char );

						if ( pi->address_type == 0 )
						{
							proxy_address_length += ( lstrlenW( pi->hostname ) + 1 ) * sizeof( wchar_t );
						}
						else// if ( si->proxy_address_type == 1 )
						{
							proxy_address_length += sizeof( unsigned long );
						}

						if ( pi->type == 3 )	// SOCKS v4
						{
							optional_extra_length += sizeof( bool );
						}
						else if ( pi->type == 4 )	// SOCKS v5
						{
							optional_extra_length += ( sizeof( bool ) * 2 );
						}

						if ( pi->username != NULL )
						{
							proxy_username_length = lstrlenA( pi->username );
							optional_extra_length += sizeof( unsigned short );
						}
						if ( pi->password != NULL )
						{
							proxy_password_length = lstrlenA( pi->password );
							optional_extra_length += sizeof( unsigned short );
						}
					}

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( ( signed )( pos +
									 url_length +
									 comments_length +
									 cookies_length +
									 headers_length +
									 data_length +
									 username_length +
									 password_length +
									 proxy_address_length +
									 proxy_username_length +
									 proxy_password_length +
									 optional_extra_length +
								   ( sizeof( int ) * 3 ) +
									 sizeof( ULONGLONG ) +
								   ( sizeof( unsigned long long ) * 3 ) +
								   ( sizeof( unsigned char ) * 5 ) +
									 sizeof( unsigned int ) +
									 sizeof( bool ) ) > size )
					{
						// Dump the buffer.
						WriteFile( hFile_downloads, buf, pos, &write, NULL );
						pos = 0;
#ifdef ENABLE_LOGGING
						lfz += write;
#endif
					}

					if ( di->shared_info->hosts > 1 )
					{
						_memcpy_s( buf + pos, size - pos, &di->downloaded, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &di->file_size, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &di->download_speed_limit, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &di->processed_header, sizeof( bool ) );
						pos += sizeof( bool );
					}

					_memcpy_s( buf + pos, size - pos, &di->parts, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );

					_memcpy_s( buf + pos, size - pos, &di->parts_limit, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );

					_memcpy_s( buf + pos, size - pos, &di->status, sizeof( unsigned int ) );
					pos += sizeof( unsigned int );

					_memcpy_s( buf + pos, size - pos, &di->code, sizeof( int ) );
					pos += sizeof( int );

					_memcpy_s( buf + pos, size - pos, &di->ssl_version, sizeof( char ) );
					pos += sizeof( char );

					_memcpy_s( buf + pos, size - pos, &di->method, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );

					_memcpy_s( buf + pos, size - pos, &di->last_modified.QuadPart, sizeof( ULONGLONG ) );
					pos += sizeof( ULONGLONG );

					_memcpy_s( buf + pos, size - pos, di->url, url_length );
					pos += url_length;

					if ( di->shared_info->hosts > 1 )
					{
						_memcpy_s( buf + pos, size - pos, di->comments, comments_length );
						pos += comments_length;
					}
					else
					{
						_memset( buf + pos, 0, sizeof( wchar_t ) );
						pos += sizeof( wchar_t );
					}

					_memcpy_s( buf + pos, size - pos, di->cookies, cookies_length );
					pos += cookies_length;

					_memcpy_s( buf + pos, size - pos, di->headers, headers_length );
					pos += headers_length;

					_memcpy_s( buf + pos, size - pos, di->data, data_length );
					pos += data_length;

					if ( di->auth_info.username != NULL )
					{
						_memcpy_s( buf + pos, size - pos, &username_length, sizeof( int ) );
						pos += sizeof( int );

						_memcpy_s( buf + pos, size - pos, di->auth_info.username, username_length );
						encode_cipher( buf + pos, username_length );
						pos += username_length;
					}
					else
					{
						_memset( buf + pos, 0, sizeof( int ) );
						pos += sizeof( int );
					}

					if ( di->auth_info.password != NULL )
					{
						_memcpy_s( buf + pos, size - pos, &password_length, sizeof( int ) );
						pos += sizeof( int );

						_memcpy_s( buf + pos, size - pos, di->auth_info.password, password_length );
						encode_cipher( buf + pos, password_length );
						pos += password_length;
					}
					else
					{
						_memset( buf + pos, 0, sizeof( int ) );
						pos += sizeof( int );
					}

					// Proxy info

					if ( pi == NULL )	// Default.
					{
						_memcpy_s( buf + pos, size - pos, 0, sizeof( char ) );
						pos += sizeof( char );
					}
					else
					{
						_memcpy_s( buf + pos, size - pos, &pi->type, sizeof( unsigned char ) );
						pos += sizeof( unsigned char );

						if ( pi->type != 0 )
						{
							_memcpy_s( buf + pos, size - pos, &pi->address_type, sizeof( unsigned char ) );
							pos += sizeof( unsigned char );

							if ( pi->address_type == 0 )
							{
								_memcpy_s( buf + pos, size - pos, pi->hostname, proxy_address_length );
								pos += proxy_address_length;
							}
							else// if ( si->proxy_address_type == 1 )
							{
								_memcpy_s( buf + pos, size - pos, &pi->ip_address, sizeof( unsigned long ) );
								pos += sizeof( unsigned long );
							}

							_memcpy_s( buf + pos, size - pos, &pi->port, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );

							if ( pi->type == 1 || pi->type == 2 )	// HTTP and HTTPS
							{
								if ( pi->username != NULL )
								{
									_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );

									_memcpy_s( buf + pos, size - pos, pi->username, proxy_username_length );
									encode_cipher( buf + pos, proxy_username_length );
									pos += proxy_username_length;
								}
								else
								{
									_memset( buf + pos, 0, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );
								}

								if ( pi->password != NULL )
								{
									_memcpy_s( buf + pos, size - pos, &proxy_password_length, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );

									_memcpy_s( buf + pos, size - pos, pi->password, proxy_password_length );
									encode_cipher( buf + pos, proxy_password_length );
									pos += proxy_password_length;
								}
								else
								{
									_memset( buf + pos, 0, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );
								}
							}
							else if ( pi->type == 3 )	// SOCKS v4
							{
								_memcpy_s( buf + pos, size - pos, &pi->resolve_domain_names, sizeof( bool ) );
								pos += sizeof( bool );

								if ( pi->username != NULL )
								{
									_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );

									_memcpy_s( buf + pos, size - pos, pi->username, proxy_username_length );
									encode_cipher( buf + pos, proxy_username_length );
									pos += proxy_username_length;
								}
								else
								{
									_memset( buf + pos, 0, sizeof( unsigned short ) );
									pos += sizeof( unsigned short );
								}
							}
							else if ( pi->type == 4 )	// SOCKS v5
							{
								_memcpy_s( buf + pos, size - pos, &pi->resolve_domain_names, sizeof( bool ) );
								pos += sizeof( bool );

								_memcpy_s( buf + pos, size - pos, &pi->use_authentication, sizeof( bool ) );
								pos += sizeof( bool );

								if ( pi->use_authentication )
								{
									if ( pi->username != NULL )
									{
										_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
										pos += sizeof( unsigned short );

										_memcpy_s( buf + pos, size - pos, pi->username, proxy_username_length );
										encode_cipher( buf + pos, proxy_username_length );
										pos += proxy_username_length;
									}
									else
									{
										_memset( buf + pos, 0, sizeof( unsigned short ) );
										pos += sizeof( unsigned short );
									}

									if ( pi->password != NULL )
									{
										_memcpy_s( buf + pos, size - pos, &proxy_password_length, sizeof( unsigned short ) );
										pos += sizeof( unsigned short );

										_memcpy_s( buf + pos, size - pos, pi->password, proxy_password_length );
										encode_cipher( buf + pos, proxy_password_length );
										pos += proxy_password_length;
									}
									else
									{
										_memset( buf + pos, 0, sizeof( unsigned short ) );
										pos += sizeof( unsigned short );
									}
								}
							}
						}
					}

					//

					unsigned char range_count = 0;
					DoublyLinkedList *range_node = di->range_list;
					while ( range_node != NULL )
					{
						++range_count;

						range_node = range_node->next;
					}

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( ( signed )( pos + sizeof( unsigned char ) + ( range_count * ( ( sizeof( unsigned long long ) * 5 ) ) ) ) > size )
					{
						// Dump the buffer.
						WriteFile( hFile_downloads, buf, pos, &write, NULL );
						pos = 0;
#ifdef ENABLE_LOGGING
						lfz += write;
#endif
					}

					_memcpy_s( buf + pos, size - pos, &range_count, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );

					range_node = di->range_list;
					while ( range_node != NULL )
					{
						RANGE_INFO *ri = ( RANGE_INFO * )range_node->data;

						//_memcpy_s( buf + pos, size - pos, ri, sizeof( RANGE_INFO ) );
						//pos += sizeof( RANGE_INFO );

						_memcpy_s( buf + pos, size - pos, &ri->range_start, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &ri->range_end, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &ri->content_length, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &ri->content_offset, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						_memcpy_s( buf + pos, size - pos, &ri->file_write_offset, sizeof( unsigned long long ) );
						pos += sizeof( unsigned long long );

						range_node = range_node->next;
					}

					host_node = host_node->next;
				}
			}

			tln = tln->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile_downloads, buf, pos, &write, NULL );
#ifdef ENABLE_LOGGING
			lfz += write;
#endif
		}

		GlobalFree( buf );

		UnlockFileEx( hFile_downloads, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_downloads );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving download history: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_download_history_csv_file( wchar_t *file_path )
{
	char ret_status = 0;

	HANDLE hFile_download_history = CreateFile( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_download_history != INVALID_HANDLE_VALUE )
	{
		int size = ( 32768 + 1 );
		int pos = 0;
		DWORD write = 0;
		char unix_timestamp[ 21 ];
		_memzero( unix_timestamp, 21 );
		char file_size[ 21 ];
		_memzero( file_size, 21 );
		char downloaded[ 21 ];
		_memzero( downloaded, 21 );

		char *write_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );
		if ( write_buf != NULL )
		{
			// Write the UTF-8 BOM and CSV column titles.
			WriteFile( hFile_download_history, "\xEF\xBB\xBF\"Filename\",\"Download Directory\",\"Date and Time Added\",\"Unix Timestamp\",\"Downloaded (bytes)\",\"File Size (bytes)\",\"URL\"", 120, &write, NULL );

			TREELISTNODE *tln = g_tree_list;
			while ( tln != NULL )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					int download_directory_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->file_path, -1, NULL, 0, NULL, NULL );
					char *utf8_download_directory = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * download_directory_length ); // Size includes the null character.
					download_directory_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->file_path, -1, utf8_download_directory, download_directory_length, NULL, NULL ) - 1;

					int filename_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->file_path + di->shared_info->filename_offset, -1, NULL, 0, NULL, NULL );
					char *utf8_filename = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * filename_length ); // Size includes the null character.
					filename_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->file_path + di->shared_info->filename_offset, -1, utf8_filename, filename_length, NULL, NULL ) - 1;

					int time_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->w_add_time, -1, NULL, 0, NULL, NULL );
					char *utf8_time = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * time_length ); // Size includes the null character.
					time_length = WideCharToMultiByte( CP_UTF8, 0, di->shared_info->w_add_time, -1, utf8_time, time_length, NULL, NULL ) - 1;

					int url_length = WideCharToMultiByte( CP_UTF8, 0, di->url, -1, NULL, 0, NULL, NULL );
					char *utf8_url = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * url_length ); // Size includes the null character.
					url_length = WideCharToMultiByte( CP_UTF8, 0, di->url, -1, utf8_url, url_length, NULL, NULL ) - 1;

					char *escaped_url = escape_csv( utf8_url );
					if ( escaped_url != NULL )
					{
						GlobalFree( utf8_url );

						utf8_url = escaped_url;
						url_length = lstrlenA( utf8_url );
					}

					// Convert the time into a 32bit Unix timestamp.
					ULARGE_INTEGER date;
					date.HighPart = di->shared_info->add_time.HighPart;
					date.LowPart = di->shared_info->add_time.LowPart;

					date.QuadPart -= ( 11644473600000 * 10000 );
		#ifdef _WIN64
					date.QuadPart /= FILETIME_TICKS_PER_SECOND;
		#else
					// Divide the 64bit value.
					__asm
					{
						xor edx, edx;				//; Zero out the register so we don't divide a full 64bit value.
						mov eax, date.HighPart;		//; We'll divide the high order bits first.
						mov ecx, FILETIME_TICKS_PER_SECOND;
						div ecx;
						mov date.HighPart, eax;		//; Store the high order quotient.
						mov eax, date.LowPart;		//; Now we'll divide the low order bits.
						div ecx;
						mov date.LowPart, eax;		//; Store the low order quotient.
						//; Any remainder will be stored in edx. We're not interested in it though.
					}
		#endif
					int timestamp_length = __snprintf( unix_timestamp, 21, "%I64u", date.QuadPart );

					int downloaded_length = __snprintf( downloaded, 21, "%I64u", di->shared_info->downloaded );
					int file_size_length = __snprintf( file_size, 21, "%I64u", di->shared_info->file_size );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( pos + filename_length + download_directory_length + time_length + timestamp_length + downloaded_length + file_size_length + url_length + 16 > size )
					{
						// Dump the buffer.
						WriteFile( hFile_download_history, write_buf, pos, &write, NULL );
						pos = 0;
					}

					// Add to the buffer.
					write_buf[ pos++ ] = '\r';
					write_buf[ pos++ ] = '\n';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_filename, filename_length );
					pos += filename_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_download_directory, download_directory_length );
					pos += download_directory_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_time, time_length );
					pos += time_length;
					write_buf[ pos++ ] = '\"';
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, unix_timestamp, timestamp_length );
					pos += timestamp_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, downloaded, downloaded_length );
					pos += downloaded_length;
					write_buf[ pos++ ] = ',';

					_memcpy_s( write_buf + pos, size - pos, file_size, file_size_length );
					pos += file_size_length;
					write_buf[ pos++ ] = ',';

					write_buf[ pos++ ] = '\"';
					_memcpy_s( write_buf + pos, size - pos, utf8_url, url_length );
					pos += url_length;
					write_buf[ pos++ ] = '\"';

					GlobalFree( utf8_download_directory );
					GlobalFree( utf8_filename );
					GlobalFree( utf8_time );
					GlobalFree( utf8_url );
				}

				tln = TLV_NextNode( tln, false );
			}

			// If there's anything remaining in the buffer, then write it to the file.
			if ( pos > 0 )
			{
				WriteFile( hFile_download_history, write_buf, pos, &write, NULL );
			}

			GlobalFree( write_buf );
		}

		CloseHandle( hFile_download_history );
	}
	else
	{
		ret_status = -1;	// Can't open file for writing.
	}

	return ret_status;
}

wchar_t *read_url_list_file( wchar_t *file_path, unsigned int &url_list_length )
{
	wchar_t *urls = NULL;

	HANDLE hFile_url_list = CreateFile( file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_url_list != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_url_list, NULL );

		// http://a.b
		if ( fz >= 10 )
		{
			char *url_list_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 1 );
			if ( url_list_buf != NULL )
			{
				BOOL bRet = ReadFile( hFile_url_list, url_list_buf, sizeof( char ) * fz, &read, NULL );
				if ( bRet != FALSE )
				{
					url_list_buf[ fz ] = 0;	// Guarantee a NULL terminated buffer.

					int length = MultiByteToWideChar( CP_UTF8, 0, url_list_buf, fz + 1, NULL, 0 );
					if ( length > 0 )
					{
						url_list_length = length - 1;
						urls = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * length );
						MultiByteToWideChar( CP_UTF8, 0, url_list_buf, fz + 1, urls, length );
					}
				}

				GlobalFree( url_list_buf );
			}
		}

		CloseHandle( hFile_url_list );
	}

	return urls;
}
