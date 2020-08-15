/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "globals.h"

#include "string_tables.h"

extern bool options_state_changed;

extern wchar_t *t_default_download_directory;
extern wchar_t *t_sound_file_path;
extern wchar_t *t_temp_download_directory;

extern FONT_SETTINGS t_odd_row_font_settings;
extern FONT_SETTINGS t_even_row_font_settings;

extern COLORREF t_background_color;
extern COLORREF t_gridline_color;
extern COLORREF t_selection_marquee_color;

extern COLORREF t_odd_row_background_color;
extern COLORREF t_even_row_background_color;

extern COLORREF t_odd_row_highlight_color;
extern COLORREF t_even_row_highlight_color;

extern COLORREF t_odd_row_highlight_font_color;
extern COLORREF t_even_row_highlight_font_color;

extern COLORREF t_progress_colors[ NUM_COLORS ];

extern COLORREF t_td_progress_colors[ TD_NUM_COLORS ];

// Free these when done.
extern wchar_t *certificate_pkcs_file_name;
extern wchar_t *certificate_cer_file_name;
extern wchar_t *certificate_key_file_name;

extern unsigned int certificate_pkcs_file_name_length;
extern unsigned int certificate_cer_file_name_length;
extern unsigned int certificate_key_file_name_length;

extern HFONT hFont_copy_proxy;
extern HFONT hFont_copy_connection;


// Options Window
extern HWND g_hWnd_options_tree;
extern HWND g_hWnd_general_tab;
extern HWND g_hWnd_appearance_tab;
extern HWND g_hWnd_connection_tab;
extern HWND g_hWnd_web_server_tab;
extern HWND g_hWnd_ftp_tab;
extern HWND g_hWnd_proxy_tab;
extern HWND g_hWnd_advanced_tab;

extern HWND g_hWnd_options_apply;


// Connection Tab
extern HWND g_hWnd_max_downloads;

extern HWND g_hWnd_retry_downloads_count;
extern HWND g_hWnd_retry_parts_count;

extern HWND g_hWnd_timeout;

extern HWND g_hWnd_max_redirects;

extern HWND g_hWnd_default_speed_limit;

extern HWND g_hWnd_default_ssl_version;
extern HWND g_hWnd_default_download_parts;

// Web Server Tab
extern HWND g_hWnd_chk_enable_server;
extern HWND g_hWnd_static_hoz1;
extern HWND g_hWnd_chk_type_server_hostname;
extern HWND g_hWnd_chk_type_server_ip_address;
extern HWND g_hWnd_server_hostname;
extern HWND g_hWnd_server_ip_address;
extern HWND g_hWnd_static_server_colon;
extern HWND g_hWnd_static_server_port;
extern HWND g_hWnd_server_port;
extern HWND g_hWnd_chk_use_authentication;
extern HWND g_hWnd_static_authentication_username;
extern HWND g_hWnd_authentication_username;
extern HWND g_hWnd_static_authentication_password;
extern HWND g_hWnd_authentication_password;
extern HWND g_hWnd_chk_authentication_type_basic;
extern HWND g_hWnd_chk_authentication_type_digest;

extern HWND g_hWnd_chk_server_enable_ssl;
extern HWND g_hWnd_static_hoz2;
extern HWND g_hWnd_chk_type_pkcs;
extern HWND g_hWnd_chk_type_pair;
extern HWND g_hWnd_static_certificate_pkcs_location;
extern HWND g_hWnd_certificate_pkcs_location;
extern HWND g_hWnd_btn_certificate_pkcs_location;
extern HWND g_hWnd_static_certificate_pkcs_password;
extern HWND g_hWnd_certificate_pkcs_password;
extern HWND g_hWnd_static_certificate_cer_location;
extern HWND g_hWnd_certificate_cer_location;
extern HWND g_hWnd_btn_certificate_cer_location;
extern HWND g_hWnd_static_certificate_key_location;
extern HWND g_hWnd_certificate_key_location;
extern HWND g_hWnd_btn_certificate_key_location;
extern HWND g_hWnd_static_server_ssl_version;
extern HWND g_hWnd_server_ssl_version;

// FTP Tab
extern HWND g_hWnd_chk_passive_mode;
extern HWND g_hWnd_chk_active_mode;
extern HWND g_hWnd_chk_fallback_mode;

extern HWND g_hWnd_static_active_listen_info;
extern HWND g_hWnd_chk_type_ftp_hostname;
extern HWND g_hWnd_chk_type_ftp_ip_address;
extern HWND g_hWnd_ftp_hostname;
extern HWND g_hWnd_ftp_ip_address;
extern HWND g_hWnd_static_ftp_port_start;
extern HWND g_hWnd_static_ftp_port_end;
extern HWND g_hWnd_ftp_port_start;
extern HWND g_hWnd_ftp_port_end;

extern HWND g_hWnd_chk_send_keep_alive;

// Proxy Tab
// HTTP proxy
extern HWND g_hWnd_chk_proxy;

extern HWND g_hWnd_ip_address;
extern HWND g_hWnd_hostname;
extern HWND g_hWnd_port;

extern HWND g_hWnd_static_port;
extern HWND g_hWnd_static_colon;

extern HWND g_hWnd_chk_type_hostname;
extern HWND g_hWnd_chk_type_ip_address;

extern HWND g_hWnd_static_auth_username;
extern HWND g_hWnd_auth_username;
extern HWND g_hWnd_static_auth_password;
extern HWND g_hWnd_auth_password;

// HTTPS proxy
extern HWND g_hWnd_chk_proxy_s;

extern HWND g_hWnd_ip_address_s;
extern HWND g_hWnd_hostname_s;
extern HWND g_hWnd_port_s;

extern HWND g_hWnd_static_port_s;
extern HWND g_hWnd_static_colon_s;

extern HWND g_hWnd_chk_type_hostname_s;
extern HWND g_hWnd_chk_type_ip_address_s;

extern HWND g_hWnd_static_auth_username_s;
extern HWND g_hWnd_auth_username_s;
extern HWND g_hWnd_static_auth_password_s;
extern HWND g_hWnd_auth_password_s;

// SOCKS proxy
extern HWND g_hWnd_chk_proxy_socks;

extern HWND g_hWnd_chk_type_socks4;
extern HWND g_hWnd_chk_type_socks5;

extern HWND g_hWnd_ip_address_socks;
extern HWND g_hWnd_hostname_socks;
extern HWND g_hWnd_port_socks;

extern HWND g_hWnd_static_port_socks;
extern HWND g_hWnd_static_colon_socks;

extern HWND g_hWnd_chk_type_hostname_socks;
extern HWND g_hWnd_chk_type_ip_address_socks;

extern HWND g_hWnd_auth_ident_username_socks;

extern HWND g_hWnd_chk_resolve_domain_names_v4a;

extern HWND g_hWnd_chk_use_authentication_socks;

extern HWND g_hWnd_static_auth_username_socks;
extern HWND g_hWnd_auth_username_socks;
extern HWND g_hWnd_static_auth_password_socks;
extern HWND g_hWnd_auth_password_socks;

extern HWND g_hWnd_chk_resolve_domain_names;

// General Tab
extern HWND g_hWnd_chk_tray_icon;
extern HWND g_hWnd_chk_minimize_to_tray;
extern HWND g_hWnd_chk_close_to_tray;
extern HWND g_hWnd_chk_start_in_tray;
extern HWND g_hWnd_chk_show_notification;

extern HWND g_hWnd_chk_always_on_top;
extern HWND g_hWnd_chk_check_for_updates_startup;
extern HWND g_hWnd_chk_enable_drop_window;
extern HWND g_hWnd_drop_window_transparency;

extern HWND g_hWnd_chk_play_sound;
extern HWND g_hWnd_sound_file;
extern HWND g_hWnd_load_sound_file;

extern HWND g_hWnd_chk_show_tray_progress;
extern HWND g_hWnd_chk_show_drop_window_progress;

// Advanced Tab
extern HWND g_hWnd_chk_download_history;
extern HWND g_hWnd_chk_quick_allocation;
extern HWND g_hWnd_chk_set_filetime;
extern HWND g_hWnd_chk_update_redirected;
extern HWND g_hWnd_chk_use_one_instance;
extern HWND g_hWnd_chk_prevent_standby;
extern HWND g_hWnd_chk_resume_downloads;

extern HWND g_hWnd_drag_and_drop_action;

extern HWND g_hWnd_prompt_last_modified;
extern HWND g_hWnd_prompt_rename;
extern HWND g_hWnd_max_file_size;
extern HWND g_hWnd_prompt_file_size;

extern HWND g_hWnd_shutdown_action;

extern HWND g_hWnd_default_download_directory;
extern HWND g_hWnd_btn_default_download_directory;

extern HWND g_hWnd_chk_temp_download_directory;
extern HWND g_hWnd_temp_download_directory;
extern HWND g_hWnd_btn_temp_download_directory;

extern HWND g_hWnd_thread_count;

// Appearance Tab

extern HWND g_hWnd_chk_show_gridlines;
extern HWND g_hWnd_chk_draw_full_rows;
extern HWND g_hWnd_chk_draw_all_rows;
extern HWND g_hWnd_chk_show_part_progress;

extern HWND g_hWnd_chk_sort_added_and_updating_items;
extern HWND g_hWnd_chk_expand_added_group_items;

//

void Enable_Disable_SSL_Windows( BOOL enable );
void Enable_Disable_Authentication_Windows( BOOL enable );
void Enable_Disable_Windows( BOOL enable );
void Set_Window_Settings();

#endif
