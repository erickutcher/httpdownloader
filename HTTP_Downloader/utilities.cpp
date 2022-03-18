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

#include "globals.h"
#include "utilities.h"
#include "string_tables.h"

#include "lite_gdi32.h"

#include "treelistview.h"

#include "dark_mode.h"

int cfg_pos_x = 0;
int cfg_pos_y = 0;
int cfg_width = MIN_WIDTH;
int cfg_height = MIN_HEIGHT;

char cfg_min_max = 0;	// 0 = normal, 1 = minimized, 2 = maximized.

int cfg_column_width1 = 35;
int cfg_column_width2 = 75;
int cfg_column_width3 = 200;
int cfg_column_width4 = 400;
int cfg_column_width5 = 110;
int cfg_column_width6 = 110;
int cfg_column_width7 = 110;
int cfg_column_width8 = 110;
int cfg_column_width9 = 25;
int cfg_column_width10 = 200;
int cfg_column_width11 = 200;
int cfg_column_width12 = 100;
int cfg_column_width13 = 90;
int cfg_column_width14 = 90;
int cfg_column_width15 = 1000;

// Column (1-15) / Virtual position (0-14)
char cfg_column_order1 = COLUMN_NUM;
char cfg_column_order2 = COLUMN_FILE_TYPE;
char cfg_column_order3 = COLUMN_FILENAME;
char cfg_column_order4 = COLUMN_DOWNLOADED;
char cfg_column_order5 = COLUMN_FILE_SIZE;
char cfg_column_order6 = COLUMN_PROGRESS;
char cfg_column_order7 = COLUMN_DOWNLOAD_SPEED;
char cfg_column_order8 = COLUMN_TIME_REMAINING;
char cfg_column_order9 = COLUMN_ACTIVE_PARTS;
char cfg_column_order10 = COLUMN_TIME_ELAPSED;
char cfg_column_order11 = COLUMN_DATE_AND_TIME_ADDED;
char cfg_column_order12 = COLUMN_DOWNLOAD_DIRECTORY;
char cfg_column_order13 = COLUMN_DOWNLOAD_SPEED_LIMIT;
char cfg_column_order14 = COLUMN_SSL_TLS_VERSION;
char cfg_column_order15 = COLUMN_URL;

bool cfg_show_toolbar = true;
bool cfg_show_column_headers = true;
bool cfg_show_status_bar = true;

unsigned char cfg_t_downloaded = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
unsigned char cfg_t_file_size = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
unsigned char cfg_t_down_speed = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
unsigned char cfg_t_speed_limit = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.

unsigned char cfg_t_status_downloaded = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
unsigned char cfg_t_status_down_speed = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
unsigned char cfg_t_status_speed_limit = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.

unsigned long long cfg_total_downloaded = 0;

int cfg_drop_pos_x = 0;	// URL drop window.
int cfg_drop_pos_y = 0;	// URL drop window.

bool cfg_tray_icon = true;
bool cfg_minimize_to_tray = false;
bool cfg_close_to_tray = false;
bool cfg_start_in_tray = false;
bool cfg_show_notification = false;

bool cfg_always_on_top = false;
bool cfg_check_for_updates = false;
bool cfg_enable_download_history = true;
bool cfg_enable_quick_allocation = false;
bool cfg_enable_sparse_file_allocation = false;
bool cfg_set_filetime = true;
bool cfg_update_redirected = false;
bool cfg_download_non_200_206 = false;
bool cfg_move_to_trash = false;
bool cfg_use_one_instance = false;
bool cfg_enable_drop_window = false;
bool cfg_prevent_standby = true;

unsigned char cfg_drag_and_drop_action = DRAG_AND_DROP_ACTION_NONE;

unsigned char cfg_shutdown_action = SHUTDOWN_ACTION_NONE;
unsigned char g_shutdown_action = SHUTDOWN_ACTION_NONE;

bool cfg_play_sound = false;
wchar_t *cfg_sound_file_path = NULL;

unsigned long cfg_thread_count = 1;	// Default is 1.
unsigned long g_max_threads = 2;	// Default is 2.

unsigned char cfg_max_downloads = 10;

unsigned char cfg_retry_downloads_count = 2;
unsigned char cfg_retry_parts_count = 0;

unsigned char cfg_default_ssl_version = 4;	// Default is TLS 1.2.
unsigned char cfg_default_download_parts = 1;

bool cfg_reallocate_parts = false;

unsigned char cfg_max_redirects = 10;

unsigned long long cfg_default_speed_limit = 0;	// 0 = Unlimited

wchar_t *cfg_default_download_directory = NULL;

unsigned int g_default_download_directory_length = 0;
unsigned int g_temp_download_directory_length = 0;

// FTP

unsigned char cfg_ftp_mode_type = 0;	// 0 = Passive, 1 = Active
bool cfg_ftp_enable_fallback_mode = false;
unsigned char cfg_ftp_address_type = 0;	// 0 = Host name, 1 = IP address
wchar_t *cfg_ftp_hostname = NULL;
unsigned long cfg_ftp_ip_address = 2130706433;	// 127.0.0.1
unsigned short cfg_ftp_port_start = 0;
unsigned short cfg_ftp_port_end = 0;

bool cfg_ftp_send_keep_alive = false;

// SFTP

bool cfg_sftp_enable_compression = false;
bool cfg_sftp_attempt_gssapi_authentication = true;
bool cfg_sftp_attempt_gssapi_key_exchange = true;
int cfg_sftp_keep_alive_time = 0;
int cfg_sftp_rekey_time = 60;
int cfg_sftp_gss_rekey_time = 2;
unsigned long cfg_sftp_rekey_data_limit = 1073741824;

// 0x40 = enabled. OR it with the enum values in putty.h.
// The order is the same as what's in the main PuTTY program.
unsigned char cfg_priority_kex_algorithm[ KEX_ALGORITHM_COUNT ] = { 0x45, 0x43, 0x42, 0x44, 0x01 };
unsigned char cfg_priority_host_key[ HOST_KEY_COUNT ] = { 0x44, 0x43, 0x41, 0x42 };
unsigned char cfg_priority_encryption_cipher[ ENCRYPTION_CIPHER_COUNT ] = { 0x43, 0x46, 0x41, 0x04, 0x02, 0x05 };

// Server

bool cfg_enable_server = false;
unsigned char cfg_server_address_type = 0;	// 0 = Host name, 1 = IP address
unsigned long cfg_server_ip_address = 2130706433;	// 127.0.0.1
wchar_t *cfg_server_hostname = NULL;
unsigned short cfg_server_port = 80;

unsigned char cfg_server_ssl_version = 2;	// TLS 1.0

bool cfg_server_enable_ssl = false;

unsigned char cfg_certificate_type = 0;	// PKCS

wchar_t *cfg_certificate_pkcs_file_name = NULL;
wchar_t *cfg_certificate_pkcs_password = NULL;

wchar_t *cfg_certificate_cer_file_name = NULL;
wchar_t *cfg_certificate_key_file_name = NULL;

bool cfg_use_authentication = false;
wchar_t *cfg_authentication_username = NULL;
wchar_t *cfg_authentication_password = NULL;
unsigned char cfg_authentication_type = AUTH_TYPE_BASIC;

// HTTP proxy
bool cfg_enable_proxy = false;
unsigned char cfg_address_type = 0;			// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname = NULL;
unsigned short cfg_port = 80;

wchar_t *cfg_proxy_auth_username = NULL;
wchar_t *cfg_proxy_auth_password = NULL;

// HTTPS proxy
bool cfg_enable_proxy_s = false;
unsigned char cfg_address_type_s = 0;			// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address_s = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname_s = NULL;
unsigned short cfg_port_s = 443;

wchar_t *cfg_proxy_auth_username_s = NULL;
wchar_t *cfg_proxy_auth_password_s = NULL;

// SOCKS proxy
bool cfg_enable_proxy_socks = false;
unsigned char cfg_socks_type = SOCKS_TYPE_V4;		// 0 = SOCKS 4, 1 = SOCKS 5
unsigned char cfg_address_type_socks = 0;			// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address_socks = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname_socks = NULL;
unsigned short cfg_port_socks = 1080;
bool cfg_use_authentication_socks = false;
bool cfg_resolve_domain_names_v4a = false;		// Proxy server resolves the domain names.
bool cfg_resolve_domain_names = false;			// Proxy server resolves the domain names.

wchar_t *cfg_proxy_auth_username_socks = NULL;
wchar_t *cfg_proxy_auth_password_socks = NULL;

wchar_t *cfg_proxy_auth_ident_username_socks = NULL;

//

unsigned short cfg_timeout = 60;

//

unsigned long long cfg_download_speed_limit = 0;	// 0 = Unlimited

//

bool cfg_resume_downloads = false;

unsigned long long cfg_max_file_size = MAX_FILE_SIZE;
unsigned char cfg_prompt_last_modified = 0;
unsigned char cfg_prompt_rename = 0;
unsigned char cfg_prompt_file_size = 0;

bool cfg_use_temp_download_directory = false;
wchar_t *cfg_temp_download_directory = NULL;

//

bool cfg_show_tray_progress = false;
unsigned char cfg_drop_window_transparency = 0x80;
bool cfg_show_drop_window_progress = false;

//

int cfg_sorted_column_index = 0;
unsigned char cfg_sorted_direction = 0;	// Sort down.
bool cfg_sort_added_and_updating_items = false;
bool cfg_expand_added_group_items = false;
bool cfg_scroll_to_last_item = false;

bool cfg_show_gridlines = true;
bool cfg_draw_full_rows = false;
bool cfg_draw_all_rows = false;
bool cfg_show_part_progress = false;

FONT_SETTINGS cfg_even_row_font_settings = { NULL };					// COLOR_WINDOWTEXT (set in SetDefaultAppearance)
FONT_SETTINGS cfg_odd_row_font_settings = { NULL };						// COLOR_WINDOWTEXT (set in SetDefaultAppearance)

#ifdef ENABLE_DARK_MODE

COLORREF cfg_background_color = RGB( 0x00, 0x00, 0x00 );
COLORREF cfg_gridline_color = RGB( 0x40, 0x40, 0x40 );
COLORREF cfg_selection_marquee_color = RGB( 0xC0, 0xC0, 0xC0 );

COLORREF cfg_even_row_background_color = RGB( 0x20, 0x20, 0x20 );
COLORREF cfg_odd_row_background_color = RGB( 0x00, 0x00, 0x00 );

COLORREF cfg_even_row_highlight_color = RGB( 0x62, 0x62, 0x62 );
COLORREF cfg_odd_row_highlight_color = RGB( 0x62, 0x62, 0x62 );

COLORREF cfg_even_row_highlight_font_color = RGB( 0xFF, 0xFF, 0xFF );	// COLOR_HIGHLIGHTTEXT (set in SetDefaultAppearance)
COLORREF cfg_odd_row_highlight_font_color = RGB( 0xFF, 0xFF, 0xFF );	// COLOR_HIGHLIGHTTEXT (set in SetDefaultAppearance)

// a = Progress Color, b = Background Color, c = Progress Font Color, d = Background Font Color, e = Border Color
// b = COLOR_WINDOW (set in SetDefaultAppearance)
COLORREF cfg_color_1a = RGB( 0x40, 0xC0, 0x40 ), cfg_color_1b = RGB( 0x00, 0x00, 0x00 ), cfg_color_1c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_1d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_1e = RGB( 0x00, 0x80, 0x00 );		//STATUS_ALLOCATING_FILE
COLORREF cfg_color_2a = RGB( 0xFF, 0x80, 0xFF ), cfg_color_2b = RGB( 0x00, 0x00, 0x00 ), cfg_color_2c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_2d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_2e = RGB( 0xA0, 0x40, 0xA0 );		//STATUS_AUTH_REQUIRED
COLORREF cfg_color_3a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_3b = RGB( 0x00, 0x00, 0x00 ), cfg_color_3c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_3d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_3e = RGB( 0x40, 0x40, 0xFF );		//STATUS_COMPLETED
COLORREF cfg_color_4a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_4b = RGB( 0x00, 0x00, 0x00 ), cfg_color_4c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_4d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_4e = RGB( 0x40, 0x40, 0xFF );		//STATUS_CONNECTING
COLORREF cfg_color_5a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_5b = RGB( 0x00, 0x00, 0x00 ), cfg_color_5c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_5d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_5e = RGB( 0x40, 0x40, 0xFF );		//STATUS_DOWNLOADING
COLORREF cfg_color_6a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_6b = RGB( 0x00, 0x00, 0x00 ), cfg_color_6c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_6d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_6e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_FAILED
COLORREF cfg_color_7a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_7b = RGB( 0x00, 0x00, 0x00 ), cfg_color_7c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_7d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_7e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_FILE_IO_ERROR
COLORREF cfg_color_8a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_8b = RGB( 0x00, 0x00, 0x00 ), cfg_color_8c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_8d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_8e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_INSUFFICIENT_DISK_SPACE
COLORREF cfg_color_9a = RGB( 0x40, 0xC0, 0x40 ), cfg_color_9b = RGB( 0x00, 0x00, 0x00 ), cfg_color_9c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_9d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_9e = RGB( 0x00, 0x80, 0x00 );		//STATUS_MOVING_FILE
COLORREF cfg_color_10a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_10b = RGB( 0x00, 0x00, 0x00 ), cfg_color_10c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_10d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_10e = RGB( 0x40, 0x40, 0xFF );	//STATUS_PAUSED
COLORREF cfg_color_11a = RGB( 0xFF, 0x80, 0xFF ), cfg_color_11b = RGB( 0x00, 0x00, 0x00 ), cfg_color_11c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_11d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_11e = RGB( 0xA0, 0x40, 0xA0 );	//STATUS_PROXY_AUTH_REQUIRED
COLORREF cfg_color_12a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_12b = RGB( 0x00, 0x00, 0x00 ), cfg_color_12c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_12d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_12e = RGB( 0x40, 0x40, 0xFF );	//STATUS_QUEUED
COLORREF cfg_color_13a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_13b = RGB( 0x00, 0x00, 0x00 ), cfg_color_13c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_13d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_13e = RGB( 0x40, 0x40, 0xFF );	//STATUS_RESTART
COLORREF cfg_color_14a = RGB( 0x80, 0x80, 0xA0 ), cfg_color_14b = RGB( 0x00, 0x00, 0x00 ), cfg_color_14c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_14d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_14e = RGB( 0x40, 0x40, 0x80 );	//STATUS_SKIPPED
COLORREF cfg_color_15a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_15b = RGB( 0x00, 0x00, 0x00 ), cfg_color_15c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_15d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_15e = RGB( 0x40, 0x40, 0xFF );	//STATUS_STOPPED
COLORREF cfg_color_16a = RGB( 0xFF, 0xB0, 0x00 ), cfg_color_16b = RGB( 0x00, 0x00, 0x00 ), cfg_color_16c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_16d = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_16e = RGB( 0xFF, 0x50, 0x00 );	//STATUS_TIMED_OUT

#else

COLORREF cfg_background_color = RGB( 0xFF, 0xFF, 0xFF );				// COLOR_WINDOW (set in SetDefaultAppearance)
COLORREF cfg_gridline_color = RGB( 0xF0, 0xF0, 0xF0 );
COLORREF cfg_selection_marquee_color = RGB( 0x00, 0x66, 0xCC );			// COLOR_HOTLIGHT (set in SetDefaultAppearance)

COLORREF cfg_even_row_background_color = RGB( 0xF7, 0xF7, 0xF7 );
COLORREF cfg_odd_row_background_color = RGB( 0xFF, 0xFF, 0xFF );		// COLOR_WINDOW (set in SetDefaultAppearance)

COLORREF cfg_even_row_highlight_color = RGB( 0x33, 0x99, 0xFF );		// COLOR_HIGHLIGHT (set in SetDefaultAppearance)
COLORREF cfg_odd_row_highlight_color = RGB( 0x33, 0x99, 0xFF );			// COLOR_HIGHLIGHT (set in SetDefaultAppearance)

COLORREF cfg_even_row_highlight_font_color = RGB( 0xFF, 0xFF, 0xFF );	// COLOR_HIGHLIGHTTEXT (set in SetDefaultAppearance)
COLORREF cfg_odd_row_highlight_font_color = RGB( 0xFF, 0xFF, 0xFF );	// COLOR_HIGHLIGHTTEXT (set in SetDefaultAppearance)

// a = Progress Color, b = Background Color, c = Progress Font Color, d = Background Font Color, e = Border Color
// b = COLOR_WINDOW (set in SetDefaultAppearance)
COLORREF cfg_color_1a = RGB( 0x40, 0xC0, 0x40 ), cfg_color_1b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_1c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_1d = RGB( 0x00, 0x00, 0x00 ), cfg_color_1e = RGB( 0x00, 0x80, 0x00 );		//STATUS_ALLOCATING_FILE
COLORREF cfg_color_2a = RGB( 0xFF, 0x80, 0xFF ), cfg_color_2b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_2c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_2d = RGB( 0x00, 0x00, 0x00 ), cfg_color_2e = RGB( 0xA0, 0x40, 0xA0 );		//STATUS_AUTH_REQUIRED
COLORREF cfg_color_3a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_3b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_3c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_3d = RGB( 0x00, 0x00, 0x00 ), cfg_color_3e = RGB( 0x40, 0x40, 0xFF );		//STATUS_COMPLETED
COLORREF cfg_color_4a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_4b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_4c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_4d = RGB( 0x00, 0x00, 0x00 ), cfg_color_4e = RGB( 0x40, 0x40, 0xFF );		//STATUS_CONNECTING
COLORREF cfg_color_5a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_5b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_5c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_5d = RGB( 0x00, 0x00, 0x00 ), cfg_color_5e = RGB( 0x40, 0x40, 0xFF );		//STATUS_DOWNLOADING
COLORREF cfg_color_6a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_6b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_6c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_6d = RGB( 0x00, 0x00, 0x00 ), cfg_color_6e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_FAILED
COLORREF cfg_color_7a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_7b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_7c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_7d = RGB( 0x00, 0x00, 0x00 ), cfg_color_7e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_FILE_IO_ERROR
COLORREF cfg_color_8a = RGB( 0xFF, 0x80, 0x80 ), cfg_color_8b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_8c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_8d = RGB( 0x00, 0x00, 0x00 ), cfg_color_8e = RGB( 0xFF, 0x20, 0x20 );		//STATUS_INSUFFICIENT_DISK_SPACE
COLORREF cfg_color_9a = RGB( 0x40, 0xC0, 0x40 ), cfg_color_9b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_9c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_9d = RGB( 0x00, 0x00, 0x00 ), cfg_color_9e = RGB( 0x00, 0x80, 0x00 );		//STATUS_MOVING_FILE
COLORREF cfg_color_10a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_10b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_10c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_10d = RGB( 0x00, 0x00, 0x00 ), cfg_color_10e = RGB( 0x40, 0x40, 0xFF );	//STATUS_PAUSED
COLORREF cfg_color_11a = RGB( 0xFF, 0x80, 0xFF ), cfg_color_11b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_11c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_11d = RGB( 0x00, 0x00, 0x00 ), cfg_color_11e = RGB( 0xA0, 0x40, 0xA0 );	//STATUS_PROXY_AUTH_REQUIRED
COLORREF cfg_color_12a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_12b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_12c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_12d = RGB( 0x00, 0x00, 0x00 ), cfg_color_12e = RGB( 0x40, 0x40, 0xFF );	//STATUS_QUEUED
COLORREF cfg_color_13a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_13b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_13c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_13d = RGB( 0x00, 0x00, 0x00 ), cfg_color_13e = RGB( 0x40, 0x40, 0xFF );	//STATUS_RESTART
COLORREF cfg_color_14a = RGB( 0x80, 0x80, 0xA0 ), cfg_color_14b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_14c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_14d = RGB( 0x00, 0x00, 0x00 ), cfg_color_14e = RGB( 0x40, 0x40, 0x80 );	//STATUS_SKIPPED
COLORREF cfg_color_15a = RGB( 0xA0, 0xA0, 0xFF ), cfg_color_15b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_15c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_15d = RGB( 0x00, 0x00, 0x00 ), cfg_color_15e = RGB( 0x40, 0x40, 0xFF );	//STATUS_STOPPED
COLORREF cfg_color_16a = RGB( 0xFF, 0xB0, 0x00 ), cfg_color_16b = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_16c = RGB( 0xFF, 0xFF, 0xFF ), cfg_color_16d = RGB( 0x00, 0x00, 0x00 ), cfg_color_16e = RGB( 0xFF, 0x50, 0x00 );	//STATUS_TIMED_OUT

#endif

COLORREF *progress_colors[ NUM_COLORS ] = { &cfg_color_1a, &cfg_color_1b, &cfg_color_1c, &cfg_color_1d, &cfg_color_1e,
											&cfg_color_2a, &cfg_color_2b, &cfg_color_2c, &cfg_color_2d, &cfg_color_2e,
											&cfg_color_3a, &cfg_color_3b, &cfg_color_3c, &cfg_color_3d, &cfg_color_3e,
											&cfg_color_4a, &cfg_color_4b, &cfg_color_4c, &cfg_color_4d, &cfg_color_4e,
											&cfg_color_5a, &cfg_color_5b, &cfg_color_5c, &cfg_color_5d, &cfg_color_5e,
											&cfg_color_6a, &cfg_color_6b, &cfg_color_6c, &cfg_color_6d, &cfg_color_6e,
											&cfg_color_7a, &cfg_color_7b, &cfg_color_7c, &cfg_color_7d, &cfg_color_7e,
											&cfg_color_8a, &cfg_color_8b, &cfg_color_8c, &cfg_color_8d, &cfg_color_8e,
											&cfg_color_9a, &cfg_color_9b, &cfg_color_9c, &cfg_color_9d, &cfg_color_9e,
											&cfg_color_10a, &cfg_color_10b, &cfg_color_10c, &cfg_color_10d, &cfg_color_10e,
											&cfg_color_11a, &cfg_color_11b, &cfg_color_11c, &cfg_color_11d, &cfg_color_11e,
											&cfg_color_12a, &cfg_color_12b, &cfg_color_12c, &cfg_color_12d, &cfg_color_12e,
											&cfg_color_13a, &cfg_color_13b, &cfg_color_13c, &cfg_color_13d, &cfg_color_13e,
											&cfg_color_14a, &cfg_color_14b, &cfg_color_14c, &cfg_color_14d, &cfg_color_14e,
											&cfg_color_15a, &cfg_color_15b, &cfg_color_15c, &cfg_color_15d, &cfg_color_15e,
											&cfg_color_16a, &cfg_color_16b, &cfg_color_16c, &cfg_color_16d, &cfg_color_16e };

COLORREF cfg_color_t_d_p = RGB( 0x00, 0xFF, 0x00 );	// Tray downloading progress
COLORREF cfg_color_t_d_b = RGB( 0x00, 0x40, 0x00 );	// Tray downloading border
COLORREF cfg_color_t_p_p = RGB( 0xFF, 0xFF, 0x00 );	// Tray paused progress
COLORREF cfg_color_t_p_b = RGB( 0x40, 0x40, 0x00 );	// Tray paused border
COLORREF cfg_color_t_e_p = RGB( 0xFF, 0x00, 0x00 );	// Tray error progress
COLORREF cfg_color_t_e_b = RGB( 0x40, 0x00, 0x00 );	// Tray error border

COLORREF cfg_color_d_d_p = RGB( 0x00, 0xFF, 0x00 );	// Drop downloading progress
COLORREF cfg_color_d_d_b = RGB( 0x00, 0x40, 0x00 );	// Drop downloading border
COLORREF cfg_color_d_p_p = RGB( 0xFF, 0xFF, 0x00 );	// Drop paused progress
COLORREF cfg_color_d_p_b = RGB( 0x40, 0x40, 0x00 );	// Drop paused border
COLORREF cfg_color_d_e_p = RGB( 0xFF, 0x00, 0x00 );	// Drop error progress
COLORREF cfg_color_d_e_b = RGB( 0x40, 0x00, 0x00 );	// Drop error border

COLORREF *td_progress_colors[ TD_NUM_COLORS ] = { &cfg_color_t_d_p, &cfg_color_t_d_b, &cfg_color_t_p_p, &cfg_color_t_p_b, &cfg_color_t_e_p, &cfg_color_t_e_b,
												  &cfg_color_d_d_p, &cfg_color_d_d_b, &cfg_color_d_p_p, &cfg_color_d_p_b, &cfg_color_d_e_p, &cfg_color_d_e_b };

char *download_columns[ NUM_COLUMNS ] = { &cfg_column_order1,
										  &cfg_column_order2,
										  &cfg_column_order3,
										  &cfg_column_order4,
										  &cfg_column_order5,
										  &cfg_column_order6,
										  &cfg_column_order7,
										  &cfg_column_order8,
										  &cfg_column_order9,
										  &cfg_column_order10,
										  &cfg_column_order11,
										  &cfg_column_order12,
										  &cfg_column_order13,
										  &cfg_column_order14,
										  &cfg_column_order15 };

int *download_columns_width[ NUM_COLUMNS ] = { &cfg_column_width1,
											   &cfg_column_width2,
											   &cfg_column_width3,
											   &cfg_column_width4,
											   &cfg_column_width5,
											   &cfg_column_width6,
											   &cfg_column_width7,
											   &cfg_column_width8,
											   &cfg_column_width9,
											   &cfg_column_width10,
											   &cfg_column_width11,
											   &cfg_column_width12,
											   &cfg_column_width13,
											   &cfg_column_width14,
											   &cfg_column_width15 };

HANDLE worker_semaphore = NULL;			// Blocks shutdown while a worker thread is active.
bool in_worker_thread = false;
bool kill_worker_thread_flag = false;	// Allow for a clean shutdown.

bool g_download_history_changed = false;

bool IsWindowsVersionOrGreater( WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor )
{
	OSVERSIONINFOEXW osvi;
	_memzero( &osvi, sizeof( OSVERSIONINFOEXW ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEXW );
	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;

	DWORDLONG const dwlConditionMask = VerSetConditionMask( VerSetConditionMask( VerSetConditionMask( 0, VER_MAJORVERSION, VER_GREATER_EQUAL ), VER_MINORVERSION, VER_GREATER_EQUAL ), VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL );

	return VerifyVersionInfoW( &osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask ) != FALSE;
}

int dllrbt_compare_a( void *a, void *b )
{
	return lstrcmpA( ( char * )a, ( char * )b );
}

int dllrbt_compare_w( void *a, void *b )
{
	return lstrcmpW( ( wchar_t * )a, ( wchar_t * )b );
}

#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 8 - ( n ) ) ) )
#define ROTATE_RIGHT( x, n ) ( ( ( x ) >> ( n ) ) | ( ( x ) << ( 8 - ( n ) ) ) )

void encode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = 0; i < buffer_length; ++i )
	{
		*buffer ^= ( unsigned char )buffer_length;
		*buffer = ( *buffer + offset ) % 256;
		*buffer = ROTATE_LEFT( ( unsigned char )*buffer, offset % 8 );

		buffer++;
		--offset;
	}
}

void decode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = buffer_length; i > 0; --i )
	{
		*buffer = ROTATE_RIGHT( ( unsigned char )*buffer, offset % 8 );
		*buffer = ( *buffer - offset ) % 256;
		*buffer ^= ( unsigned char )buffer_length;

		buffer++;
		--offset;
	}
}

wchar_t *GetMonth( unsigned short month )
{
	if ( month > 12 || month < 1 )
	{
		return L"";
	}

	//return month_string_table[ month - 1 ].value;
	return g_locale_table[ MONTH_STRING_TABLE_OFFSET + ( month - 1 ) ].value;
}

wchar_t *GetDay( unsigned short day )
{
	if ( day > 6 )
	{
		return L"";
	}

	//return day_string_table[ day ].value;
	return g_locale_table[ DAY_STRING_TABLE_OFFSET + day ].value;
}

void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st )
{
	FILETIME ft;
	LARGE_INTEGER li;
	li.QuadPart = Int32x32To64( t, 10000000 ) + 116444736000000000;

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	FileTimeToSystemTime( &ft, st );
}

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns )
{
	for ( unsigned char i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] == -1 )	// See which columns are disabled.
		{
			for ( unsigned char j = 0; j < total_columns; ++j )
			{
				if ( arr[ j ] >= i )	// Increment each virtual column that comes after the disabled column.
				{
					arr[ j ]++;
				}
			}
		}
	}
}

int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( count == column_index )
			{
				return i;
			}
			else
			{
				++count;
			}
		}
	}

	return -1;
}

int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( *column_arr[ i ] == virtual_index )
			{
				return count;
			}

			++count;
		}
	}

	return -1;
}

void SetDefaultColumnOrder()
{
	cfg_column_order1 = COLUMN_NUM;
	cfg_column_order2 = COLUMN_FILE_TYPE;
	cfg_column_order3 = COLUMN_FILENAME;
	cfg_column_order4 = COLUMN_DOWNLOADED;
	cfg_column_order5 = COLUMN_FILE_SIZE;
	cfg_column_order6 = COLUMN_PROGRESS;
	cfg_column_order7 = COLUMN_DOWNLOAD_SPEED;
	cfg_column_order8 = COLUMN_TIME_REMAINING;
	cfg_column_order9 = COLUMN_ACTIVE_PARTS;
	cfg_column_order10 = COLUMN_TIME_ELAPSED;
	cfg_column_order11 = COLUMN_DATE_AND_TIME_ADDED;
	cfg_column_order12 = COLUMN_DOWNLOAD_DIRECTORY;
	cfg_column_order13 = COLUMN_DOWNLOAD_SPEED_LIMIT;
	cfg_column_order14 = COLUMN_SSL_TLS_VERSION;
	cfg_column_order15 = COLUMN_URL;
}

void UpdateColumnOrders()
{
	int arr[ NUM_COLUMNS ];
	int offset = 0;

	_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS; ++i )
	{
		if ( *download_columns[ i ] != -1 )
		{
			*download_columns[ i ] = ( char )arr[ offset++ ];
		}
	}
}

void CheckColumnOrders( char *column_arr[], unsigned char num_columns )
{
	// Make sure the first column is always 0 or -1.
	if ( *column_arr[ 0 ] > 0 || *column_arr[ 0 ] < -1 )
	{
		SetDefaultColumnOrder();
		return;
	}

	// Look for duplicates, or values that are out of range.
	unsigned char *is_set = ( unsigned char * )GlobalAlloc( GPTR, sizeof( unsigned char ) * num_columns );
	if ( is_set != NULL )
	{
		// Check every other column.
		for ( int i = 1; i < num_columns; ++i )
		{
			if ( *column_arr[ i ] != -1 )
			{
				if ( *column_arr[ i ] < num_columns )
				{
					if ( is_set[ *column_arr[ i ] ] == 0 )
					{
						is_set[ *column_arr[ i ] ] = 1;
					}
					else	// Revert duplicate values.
					{
						SetDefaultColumnOrder();

						break;
					}
				}
				else	// Revert out of range values.
				{
					SetDefaultColumnOrder();

					break;
				}
			}
		}

		GlobalFree( is_set );
	}
}

void CheckColumnWidths()
{
	if ( cfg_column_width1 < 0 || cfg_column_width1 > 2560 ) { cfg_column_width1 = 35; }
	if ( cfg_column_width2 < 0 || cfg_column_width2 > 2560 ) { cfg_column_width2 = 75; }
	if ( cfg_column_width3 < 0 || cfg_column_width3 > 2560 ) { cfg_column_width3 = 200; }
	if ( cfg_column_width4 < 0 || cfg_column_width4 > 2560 ) { cfg_column_width4 = 400; }
	if ( cfg_column_width5 < 0 || cfg_column_width5 > 2560 ) { cfg_column_width5 = 110; }
	if ( cfg_column_width6 < 0 || cfg_column_width6 > 2560 ) { cfg_column_width6 = 110; }
	if ( cfg_column_width7 < 0 || cfg_column_width7 > 2560 ) { cfg_column_width7 = 110; }
	if ( cfg_column_width8 < 0 || cfg_column_width8 > 2560 ) { cfg_column_width8 = 110; }
	if ( cfg_column_width9 < 0 || cfg_column_width9 > 2560 ) { cfg_column_width9 = 25; }
	if ( cfg_column_width10 < 0 || cfg_column_width10 > 2560 ) { cfg_column_width10 = 200; }
	if ( cfg_column_width11 < 0 || cfg_column_width11 > 2560 ) { cfg_column_width11 = 200; }
	if ( cfg_column_width12 < 0 || cfg_column_width12 > 2560 ) { cfg_column_width12 = 100; }
	if ( cfg_column_width13 < 0 || cfg_column_width13 > 2560 ) { cfg_column_width13 = 90; }
	if ( cfg_column_width14 < 0 || cfg_column_width14 > 2560 ) { cfg_column_width14 = 90; }
	if ( cfg_column_width15 < 0 || cfg_column_width15 > 2560 ) { cfg_column_width15 = 1000; }
}

void SetDefaultAppearance()
{
	//_memzero( &cfg_even_row_font_settings.lf, sizeof( LOGFONT ) );
	//_memzero( &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ) );

	cfg_odd_row_highlight_font_color = cfg_even_row_highlight_font_color = ( COLORREF )_GetSysColor( COLOR_HIGHLIGHTTEXT );

#ifdef ENABLE_DARK_MODE

	cfg_odd_row_font_settings.font_color = cfg_even_row_font_settings.font_color = ( COLORREF )RGB( 0xFF, 0xFF, 0xFF );

#else

	cfg_odd_row_font_settings.font_color = cfg_even_row_font_settings.font_color = ( COLORREF )_GetSysColor( COLOR_WINDOWTEXT );

	cfg_even_row_highlight_color = cfg_odd_row_highlight_color = ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT );

	cfg_background_color = cfg_odd_row_background_color = ( COLORREF )_GetSysColor( COLOR_WINDOW );

#endif

	// Windows XP's color is different. Let's just hardcode it.
	//cfg_selection_marquee_color = ( COLORREF )_GetSysColor( COLOR_HOTLIGHT );

	cfg_color_1b = cfg_odd_row_background_color;
	cfg_color_2b = cfg_odd_row_background_color;
	cfg_color_3b = cfg_odd_row_background_color;
	cfg_color_4b = cfg_odd_row_background_color;
	cfg_color_5b = cfg_odd_row_background_color;
	cfg_color_6b = cfg_odd_row_background_color;
	cfg_color_7b = cfg_odd_row_background_color;
	cfg_color_8b = cfg_odd_row_background_color;
	cfg_color_9b = cfg_odd_row_background_color;
	cfg_color_10b = cfg_odd_row_background_color;
	cfg_color_11b = cfg_odd_row_background_color;
	cfg_color_12b = cfg_odd_row_background_color;
	cfg_color_13b = cfg_odd_row_background_color;
	cfg_color_14b = cfg_odd_row_background_color;
	cfg_color_15b = cfg_odd_row_background_color;
}

void AdjustRowHeight()
{
	int height1, height2;
	TEXTMETRIC tm;
	HDC hDC = _GetDC( NULL );
	HFONT ohf = ( HFONT )_SelectObject( hDC, cfg_odd_row_font_settings.font );
	_GetTextMetricsW( hDC, &tm );
	height1 = tm.tmHeight + tm.tmExternalLeading + 5;
	_SelectObject( hDC, ohf );	// Reset old font.
	ohf = ( HFONT )_SelectObject( hDC, cfg_even_row_font_settings.font );
	_GetTextMetricsW( hDC, &tm );
	height2 = tm.tmHeight + tm.tmExternalLeading + 5;
	_SelectObject( hDC, ohf );	// Reset old font.
	_ReleaseDC( NULL, hDC );

	g_row_height = ( height1 > height2 ? height1 : height2 );

	int icon_height = _GetSystemMetrics( SM_CYSMICON ) + 2;
	if ( g_row_height < icon_height )
	{
		g_row_height = icon_height;
	}
}

// Must use GlobalFree on this.
char *GlobalStrDupA( const char *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenA( _Str ) + sizeof( char );

	char *ret = ( char * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

// Must use GlobalFree on this.
wchar_t *GlobalStrDupW( const wchar_t *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenW( _Str ) * sizeof( wchar_t ) + sizeof( wchar_t );

	wchar_t *ret = ( wchar_t * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

char *strnchr( const char *s, int c, int n )
{
	if ( s == NULL )
	{
		return NULL;
	}

	while ( *s != NULL && n-- > 0 )
	{
		if ( *s == c )
		{
			return ( ( char * )s );
		}
		++s;
	}
	
	return NULL;
}

// Default is base 10.
unsigned long long strtoull( char *str, bool base16 )
{
	if ( str == NULL )
	{
		return 0;
	}

	char *p = str;

	ULARGE_INTEGER uli;
	uli.QuadPart = 0;

	unsigned char digit = 0;

	if ( !base16 )
	{
		while ( *p && ( *p >= '0' && *p <= '9' ) )
		{
			if ( uli.QuadPart > ( ULLONG_MAX / 10 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 10;

			/*__asm
			{
				mov     eax, dword ptr [ uli.QuadPart + 4 ]
				cmp		eax, 0					;// See if our QuadPart's value extends to 64 bits.
				mov     ecx, 10					;// Store the base (10) multiplier (low order bits).
				jne     short hard10			;// If there are high order bits in QuadPart, then multiply/add high and low bits.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store the QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits.

				jmp		finish10				;// Store the results in our 64 bit value.

			hard10:

				push    ebx						;// Save value to stack.

				mul     ecx						;// Multiply the high order bits of QuadPart with the low order bits of base (10).
				mov     ebx, eax				;// Store the result.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits of QuadPart with the low order bits of base (10). edx = high, eax = low
				add     edx, ebx				;// Add the low order bits (ebx) to the high order bits (edx).

				pop     ebx						;// Restore value from stack.

			finish10:

				mov		uli.HighPart, edx		;// Store the high order bits.
				mov		uli.LowPart, eax		;// Store the low order bits.
			}*/

			digit = *p - '0';

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}
	else
	{
		while ( *p )
		{
			if ( *p >= '0' && *p <= '9' )
			{
				digit = *p - '0';
			}
			else if ( *p >= 'a' && *p <= 'f' )
			{
				digit = *p - 'a' + 10;
			}
			else if ( *p >= 'A' && *p <= 'F' )
			{
				digit = *p - 'A' + 10;
			}
			else
			{
				break;
			}

			if ( uli.QuadPart > ( ULLONG_MAX / 16 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 16;

			/*__asm
			{
				mov     eax, dword ptr [ uli.QuadPart + 4 ]
				cmp		eax, 0					;// See if our QuadPart's value extends to 64 bits.
				mov     ecx, 16					;// Store the base (16) multiplier (low order bits).
				jne     short hard16			;// If there are high order bits in QuadPart, then multiply/add high and low bits.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store the QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits.

				jmp		finish16				;// Store the results in our 64 bit value.

			hard16:

				push    ebx						;// Save value to stack.

				mul     ecx						;// Multiply the high order bits of QuadPart with the low order bits of base (16).
				mov     ebx, eax				;// Store the result.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits of QuadPart with the low order bits of base (16). edx = high, eax = low
				add     edx, ebx				;// Add the low order bits (ebx) to the high order bits (edx).

				pop     ebx						;// Restore value from stack.

			finish16:

				mov		uli.HighPart, edx		;// Store the high order bits.
				mov		uli.LowPart, eax		;// Store the low order bits.
			}*/

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}

	return uli.QuadPart;
}

// Default is base 10.
unsigned long long wcstoull( wchar_t *str, bool base16 )
{
	if ( str == NULL )
	{
		return 0;
	}

	wchar_t *p = str;

	ULARGE_INTEGER uli;
	uli.QuadPart = 0;

	wchar_t digit = 0;

	if ( !base16 )
	{
		while ( *p && ( *p >= L'0' && *p <= L'9' ) )
		{
			if ( uli.QuadPart > ( ULLONG_MAX / 10 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 10;

			digit = *p - L'0';

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}
	else
	{
		while ( *p )
		{
			if ( *p >= L'0' && *p <= L'9' )
			{
				digit = *p - L'0';
			}
			else if ( *p >= L'a' && *p <= L'f' )
			{
				digit = *p - L'a' + 10;
			}
			else if ( *p >= L'A' && *p <= L'F' )
			{
				digit = *p - L'A' + 10;
			}
			else
			{
				break;
			}

			if ( uli.QuadPart > ( ULLONG_MAX / 16 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 16;

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}

	return uli.QuadPart;
}

void EscapeFilename( wchar_t *filename )
{
	while ( filename != NULL && *filename != NULL )
	{
		if ( *filename == L'\\' ||
			 *filename == L'/' ||
			 *filename == L':' ||
			 *filename == L'*' ||
			 *filename == L'?' ||
			 *filename == L'\"' ||
			 *filename == L'<' ||
			 *filename == L'>' ||
			 *filename == L'|' )
		{
			*filename = L'_';
		}

		++filename;
	}
}

// Returns the position of the file extension if found, or the length of the filename if not found.
unsigned long get_file_extension_offset( wchar_t *filename, unsigned long length )
{
	unsigned long tmp_length = length;

	while ( tmp_length != 0 )
	{
		if ( filename[ --tmp_length ] == L'.' )
		{
			return tmp_length;
		}
	}

	return length;
}

/*wchar_t *get_extension_from_filename( wchar_t *filename, unsigned long length )
{
	while ( length != 0 && filename[ --length ] != L'.' );

	return filename + length;
}*/

void GetDownloadFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] )
{
	if ( di != NULL )
	{
		_wmemcpy_s( file_path, MAX_PATH, di->shared_info->file_path, MAX_PATH );
		if ( di->shared_info->filename_offset > 0 )
		{
			file_path[ di->shared_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
		}
	}
}

int GetTemporaryFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] )
{
	int filename_length = 0;

	if ( di != NULL )
	{
		filename_length = lstrlenW( di->shared_info->file_path + di->shared_info->filename_offset );

		_wmemcpy_s( file_path, MAX_PATH, cfg_temp_download_directory, g_temp_download_directory_length );
		file_path[ g_temp_download_directory_length ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
		_wmemcpy_s( file_path + ( g_temp_download_directory_length + 1 ), MAX_PATH - ( g_temp_download_directory_length - 1 ), di->shared_info->file_path + di->shared_info->filename_offset, filename_length );
		file_path[ g_temp_download_directory_length + filename_length + 1 ] = 0;	// Sanity.
	}

	return filename_length;
}

int GetDomainParts( wchar_t *site, wchar_t *offsets[ 128 ] )
{
	int count = 0;
	wchar_t *ptr = site;
	wchar_t *ptr_s = ptr;

	while ( ptr != NULL && count < 127 )
	{
		if ( *ptr == L'.' )
		{
			offsets[ count++ ] = ptr_s;

			ptr_s = ptr + 1;
		}
		else if ( *ptr == NULL )
		{
			offsets[ count++ ] = ptr_s;

			break;
		}

		++ptr;
	}

	if ( ptr != NULL )
	{
		offsets[ count ] = ptr;	// End of string.
	}

	return count;
}

char *GetUTF8Domain( wchar_t *domain )
{
	int domain_length = WideCharToMultiByte( CP_UTF8, 0, domain, -1, NULL, 0, NULL, NULL );
	char *utf8_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * domain_length ); // Size includes the null character.
	WideCharToMultiByte( CP_UTF8, 0, domain, -1, utf8_domain, domain_length, NULL, NULL );

	return utf8_domain;
}

// Allocates a new string if characters need escaping. Otherwise, it returns NULL.
char *escape_csv( const char *string )
{
	char *escaped_string = NULL;
	char *q = NULL;
	const char *p = NULL;
	int c = 0;

	if ( string == NULL )
	{
		return NULL;
	}

	// Get the character count and offset it for any quotes.
	for ( c = 0, p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			++c;
		}
		else
		{
			c += 2;
		}
	}

	// If the string has no special characters to escape, then return NULL.
	if ( c <= ( p - string ) )
	{
		return NULL;
	}

	q = escaped_string = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( c + 1 ) );
	if ( q != NULL )
	{
		for ( p = string; *p != NULL; ++p ) 
		{
			if ( *p != '\"' )
			{
				*q = *p;
				++q;
			}
			else
			{
				*q++ = '\"';
				*q++ = '\"';
			}
		}

		*q = 0;	// Sanity.
	}

	return escaped_string;
}

char to_hex_a( char code )
{
	static char hex[] = "0123456789ABCDEF";
	return hex[ code & 15 ];
}

wchar_t to_hex_w( char code )
{
	static wchar_t hex[] = L"0123456789ABCDEF";
	return hex[ code & 15 ];
}


char from_hex_a( char c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return is_digit_a( *c ) ? *c - '0' : *c - 'a' + 10;

	if ( is_digit_a( c ) )
	{
		return c - '0';
	}
	else if ( c - 'a' + 0U < 6U )
	{
		return c - 'a' + 10;
	}
	else if ( c - 'A' + 0U < 6U )
	{
		return c - 'A' + 10;
	}

	return c;
}

wchar_t from_hex_w( wchar_t c )
{
	//_CharLowerBuffW( ( LPWSTR )c, 1 );
	//return is_digit_w( *c ) ? *c - L'0' : *c - L'a' + 10;

	if ( is_digit_w( c ) )
	{
		return c - L'0';
	}
	else if ( c - L'a' + 0U < 6U )
	{
		return c - L'a' + 10;
	}
	else if ( c - L'A' + 0U < 6U )
	{
		return c - L'A' + 10;
	}

	return c;
}

bool is_hex_a( char c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return ( is_digit_a( *c ) || ( *c - 'a' + 0U < 6U ) );

	return ( is_digit_a( c ) || ( c - 'a' + 0U < 6U ) || ( c - 'A' + 0U < 6U ) );
}

bool is_hex_w( wchar_t c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return ( is_digit_w( *c ) || ( *c - 'a' + 0U < 6U ) );

	return ( is_digit_w( c ) || ( c - L'a' + 0U < 6U ) || ( c - L'A' + 0U < 6U ) );
}

char *url_encode_a( char *str, unsigned int str_len, unsigned int *enc_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( str_len * 3 ) + 1 ) );
	char *pbuf = buf;

	if ( pbuf != NULL )
	{
		while ( pstr < ( str + str_len ) )
		{
			/*if ( _IsCharAlphaNumericA( *pstr ) )
			{
				*pbuf++ = *pstr;
			}
			else if ( *pstr == ' ' )
			{
				*pbuf++ = '%';
				*pbuf++ = '2';
				*pbuf++ = '0';
			}
			else
			{
				*pbuf++ = '%';
				*pbuf++ = to_hex_a( *pstr >> 4 );
				*pbuf++ = to_hex_a( *pstr & 15 );
			}*/

			if ( *pstr == ' ' ||
				 *pstr == '<' ||
				 *pstr == '>' ||
				 *pstr == '#' ||
				 *pstr == '%' ||
				 *pstr == '\"' ||
				 *pstr == '{' ||
				 *pstr == '}' ||
				 *pstr == '|' ||
				 *pstr == '\\' ||
				 *pstr == '^' ||
				 *pstr == '[' ||
				 *pstr == ']' ||
				 *pstr == '`' ||
				 *pstr == 0x7F ||
				 ( *pstr >= 0x00 && *pstr <= 0x1F ) )
			{
				*pbuf++ = L'%';
				*pbuf++ = to_hex_a( *pstr >> 4 );
				*pbuf++ = to_hex_a( *pstr & 15 );
			}
			else
			{
				*pbuf++ = *pstr;
			}

			++pstr;
		}

		*pbuf = '\0';

		if ( enc_len != NULL )
		{
			*enc_len = ( unsigned int )( pbuf - buf );
		}
	}

	return buf;
}

wchar_t *url_encode_w( wchar_t *str, unsigned int str_len, unsigned int *enc_len )
{
	wchar_t *pstr = str;
	wchar_t *buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( ( str_len * 3 ) + 1 ) );
	wchar_t *pbuf = buf;

	if ( pbuf != NULL )
	{
		while ( pstr < ( str + str_len ) )
		{
			/*if ( _IsCharAlphaNumericW( *pstr ) )
			{
				*pbuf++ = *pstr;
			}
			else if ( *pstr == L' ' )
			{
				*pbuf++ = L'%';
				*pbuf++ = L'2';
				*pbuf++ = L'0';
			}
			else
			{
				*pbuf++ = L'%';
				*pbuf++ = to_hex_w( *pstr >> 4 );
				*pbuf++ = to_hex_w( *pstr & 15 );
			}*/

			if ( *pstr == L' ' ||
				 *pstr == L'<' ||
				 *pstr == L'>' ||
				 *pstr == L'#' ||
				 *pstr == L'%' ||
				 *pstr == L'\"' ||
				 *pstr == L'{' ||
				 *pstr == L'}' ||
				 *pstr == L'|' ||
				 *pstr == L'\\' ||
				 *pstr == L'^' ||
				 *pstr == L'[' ||
				 *pstr == L']' ||
				 *pstr == L'`' ||
				 *pstr == 0x7F ||
				 ( *pstr >= 0x00 && *pstr <= 0x1F ) )
			{
				*pbuf++ = L'%';
				*pbuf++ = to_hex_w( ( char )( *pstr >> 4 ) );
				*pbuf++ = to_hex_w( *pstr & 15 );
			}
			else
			{
				*pbuf++ = *pstr;
			}

			++pstr;
		}

		*pbuf = L'\0';

		if ( enc_len != NULL )
		{
			*enc_len = ( unsigned int )( pbuf - buf );
		}
	}

	return buf;
}

char *url_decode_a( char *str, unsigned int str_len, unsigned int *dec_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( str_len + 1 ) );
	char *pbuf = buf;

	if ( pbuf != NULL )
	{
		while ( pstr < ( str + str_len ) )
		{
			if ( *pstr == '%' )
			{
				// Look at the next two characters.
				if ( ( ( pstr + 3 ) <= ( str + str_len ) ) )
				{
					// See if they're both hex values.
					if ( ( pstr[ 1 ] != NULL && is_hex_a( pstr[ 1 ] ) ) &&
						 ( pstr[ 2 ] != NULL && is_hex_a( pstr[ 2 ] ) ) )
					{
						*pbuf++ = from_hex_a( pstr[ 1 ] ) << 4 | from_hex_a( pstr[ 2 ] );
						pstr += 2;
					}
					else
					{
						*pbuf++ = *pstr;
					}
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			/*else if ( *pstr == '+' )
			{ 
				*pbuf++ = ' ';
			}*/
			else
			{
				*pbuf++ = *pstr;
			}

			++pstr;
		}

		*pbuf = '\0';

		if ( dec_len != NULL )
		{
			*dec_len = ( unsigned int )( pbuf - buf );
		}
	}

	return buf;
}

wchar_t *url_decode_w( wchar_t *str, unsigned int str_len, unsigned int *dec_len )
{
	wchar_t *pstr = str;
	wchar_t *buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( str_len + 1 ) );
	wchar_t *pbuf = buf;

	if ( pbuf != NULL )
	{
		while ( pstr < ( str + str_len ) )
		{
			if ( *pstr == L'%' )
			{
				// Look at the next two characters.
				if ( ( ( pstr + 3 ) <= ( str + str_len ) ) )
				{
					// See if they're both hex values.
					if ( ( pstr[ 1 ] != NULL && is_hex_w( pstr[ 1 ] ) ) &&
						 ( pstr[ 2 ] != NULL && is_hex_w( pstr[ 2 ] ) ) )
					{
						*pbuf++ = from_hex_w( pstr[ 1 ] ) << 4 | from_hex_w( pstr[ 2 ] );
						pstr += 2;
					}
					else
					{
						*pbuf++ = *pstr;
					}
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			/*else if ( *pstr == L'+' )
			{ 
				*pbuf++ = L' ';
			}*/
			else
			{
				*pbuf++ = *pstr;
			}

			++pstr;
		}

		*pbuf = L'\0';

		if ( dec_len != NULL )
		{
			*dec_len = ( unsigned int )( pbuf - buf );
		}
	}

	return buf;
}

char *html_entity_decode_a( char *str, unsigned int str_len, unsigned int *dec_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( str_len + 1 ) );
	char *pbuf = buf;

	if ( pbuf != NULL )
	{
		while ( pstr < ( str + str_len ) )
		{
			if ( *pstr == '&' )
			{
				// Look at the next two characters.
				if ( ( ( pstr + 5 ) <= ( str + str_len ) ) )
				{
					// See if they're both hex values.
					if ( ( pstr[ 1 ] != NULL && pstr[ 1 ] == 'a' ) &&
						 ( pstr[ 2 ] != NULL && pstr[ 2 ] == 'm' ) &&
						 ( pstr[ 3 ] != NULL && pstr[ 3 ] == 'p' ) &&
						 ( pstr[ 4 ] != NULL && pstr[ 4 ] == ';' ) )
					{
						*pbuf++ = '&';
						pstr += 4;
					}
					else
					{
						*pbuf++ = *pstr;
					}
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			/*else if ( *pstr == '+' )
			{ 
				*pbuf++ = ' ';
			}*/
			else
			{
				*pbuf++ = *pstr;
			}

			++pstr;
		}

		*pbuf = '\0';

		if ( dec_len != NULL )
		{
			*dec_len = ( unsigned int )( pbuf - buf );
		}
	}

	return buf;
}

void kill_worker_thread()
{
	if ( in_worker_thread )
	{
		// This semaphore will be released when the thread gets killed.
		worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_worker_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( worker_semaphore, 5000 );
		CloseHandle( worker_semaphore );
		worker_semaphore = NULL;
	}
}

// This will allow our main thread to continue while secondary threads finish their processing.
THREAD_RETURN cleanup( void * /*pArguments*/ )
{
	kill_worker_thread();

	// DestroyWindow won't work on a window from a different thread. So we'll send a message to trigger it.
	_SendMessageW( g_hWnd_main, WM_DESTROY_ALT, 0, 0 );

	_ExitThread( 0 );
	//return 0;
}

void FreeCommandLineArgs( CL_ARGS **cla )
{
	if ( *cla != NULL )
	{
		if ( ( *cla )->download_directory ) { GlobalFree( ( *cla )->download_directory ); }
		if ( ( *cla )->download_history_file ) { GlobalFree( ( *cla )->download_history_file ); }
		if ( ( *cla )->url_list_file ) { GlobalFree( ( *cla )->url_list_file ); }
		if ( ( *cla )->urls ) { GlobalFree( ( *cla )->urls ); }
		if ( ( *cla )->cookies ) { GlobalFree( ( *cla )->cookies ); }
		if ( ( *cla )->headers ) { GlobalFree( ( *cla )->headers ); }
		if ( ( *cla )->data ) { GlobalFree( ( *cla )->data ); }
		if ( ( *cla )->username ) { GlobalFree( ( *cla )->username ); }
		if ( ( *cla )->password ) { GlobalFree( ( *cla )->password ); }
		if ( ( *cla )->proxy_hostname ) { GlobalFree( ( *cla )->proxy_hostname ); }
		if ( ( *cla )->proxy_username ) { GlobalFree( ( *cla )->proxy_username ); }
		if ( ( *cla )->proxy_password ) { GlobalFree( ( *cla )->proxy_password ); }

		GlobalFree( *cla );

		*cla = NULL;
	}
}

char *CreateMD5( BYTE *input, DWORD input_len )
{
	char *md5 = NULL;

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			if ( _CryptHashData( hHash, input, input_len, 0 ) )
			{
				DWORD cbHash = MD5_LENGTH;
				BYTE Hash[ MD5_LENGTH ];

				if ( _CryptGetHashParam( hHash, HP_HASHVAL, Hash, &cbHash, 0 ) )
				{
					DWORD md5_length = cbHash * 2;
					md5 = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( md5_length + 1 ) );
					if ( md5 != NULL )
					{
						CHAR digits[] = "0123456789abcdef";
						for ( DWORD i = 0; i < cbHash; ++i )
						{
							__snprintf( md5 + ( 2 * i ), md5_length - ( 2 * i ), "%c%c", digits[ Hash[ i ] >> 4 ], digits[ Hash[ i ] & 0xF ] );
						}
						md5[ md5_length ] = 0;	// Sanity.
					}
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	return md5;
}

void CreateCNonce( char **cnonce, DWORD *cnonce_length )
{
	*cnonce = NULL;
	*cnonce_length = 0;

	HCRYPTPROV hProvider = NULL;

	if ( _CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT ) )
	{
		BYTE rbuffer[ 8 ];
		_memzero( rbuffer, 8 );

		if ( _CryptGenRandom( hProvider, 8, ( BYTE * )&rbuffer ) )
		{
			*cnonce = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( 16 + 1 ) );
			if ( *cnonce != NULL )
			{
				*cnonce_length = 16;

				CHAR digits[] = "0123456789abcdef";
				for ( DWORD i = 0; i < 8; ++i )
				{
					__snprintf( *cnonce + ( 2 * i ), *cnonce_length - ( 2 * i ), "%c%c", digits[ rbuffer[ i ] >> 4 ], digits[ rbuffer[ i ] & 0xF ] );
				}
				*( *cnonce + *cnonce_length ) = 0;	// Sanity.
			}
		}
	}

	if ( hProvider != NULL )
	{
		_CryptReleaseContext( hProvider, 0 );
	}
}

void GetMD5String( HCRYPTHASH *hHash, char **md5, DWORD *md5_length )
{
	DWORD cbHash = MD5_LENGTH;
	BYTE Hash[ MD5_LENGTH ];

	*md5 = NULL;
	*md5_length = 0;

	if ( _CryptGetHashParam( *hHash, HP_HASHVAL, Hash, &cbHash, 0 ) )
	{
		*md5 = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( ( cbHash * 2 ) + 1 ) );
		if ( *md5 != NULL )
		{
			*md5_length = cbHash * 2;

			CHAR digits[] = "0123456789abcdef";
			for ( DWORD i = 0; i < cbHash; ++i )
			{
				__snprintf( *md5 + ( 2 * i ), *md5_length - ( 2 * i ), "%c%c", digits[ Hash[ i ] >> 4 ], digits[ Hash[ i ] & 0xF ] );
			}
			*( *md5 + *md5_length ) = 0;	// Sanity.
		}
	}
}

void CreateDigestAuthorizationInfo( char **nonce, unsigned long &nonce_length, char **opaque, unsigned long &opaque_length )
{
	char *HA1 = NULL;

	if ( *nonce != NULL )
	{
		GlobalFree( *nonce );
		*nonce = NULL;
	}

	nonce_length = 0;

	if ( *opaque != NULL )
	{
		GlobalFree( *opaque );
		*opaque = NULL;
	}

	opaque_length = 0;

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		BYTE rbuffer[ 16 ];

		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptGenRandom( hProv, 16, ( BYTE * )&rbuffer );

			_CryptHashData( hHash, rbuffer, 16, 0 );

			GetMD5String( &hHash, nonce, &nonce_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptGenRandom( hProv, 16, ( BYTE * )&rbuffer );

			_CryptHashData( hHash, rbuffer, 16, 0 );

			GetMD5String( &hHash, opaque, &opaque_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
}

void CreateDigestAuthorizationKey( char *username, char *password, char *method, char *resource, AUTH_INFO *auth_info, char **auth_key, DWORD *auth_key_length )
{
	*auth_key = NULL;
	*auth_key_length = 0;
	DWORD auth_key_offset = 0;

	char *HA1 = NULL;
	DWORD HA1_length = 0;

	char *HA2 = NULL;
	DWORD HA2_length = 0;

	char *response = NULL;
	DWORD response_length = 0;

	char *cnonce = NULL;
	DWORD cnonce_length = 0;

	int username_length = 0;
	int password_length = 0;
	int realm_length = 0;

	int nonce_length = 0;
	int uri_length = 0;
	int opaque_length = 0;
	int qop_length = 0;

	int method_length = 0;

	++auth_info->nc;	// Update regardless of whether we have a qop value. Don't want to get in an infinite loop.

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		// If auth_info.algorithm is not set, then assume it's MD5.

		// Create HA1.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			nonce_length = lstrlenA( auth_info->nonce );

			username_length = lstrlenA( username );
			password_length = lstrlenA( password );
			realm_length = lstrlenA( auth_info->realm );

			_CryptHashData( hHash, ( BYTE * )username, username_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->realm, realm_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )password, password_length, 0 );

			GetMD5String( &hHash, &HA1, &HA1_length );

			// MD5-sess
			if ( auth_info->algorithm == 2 )
			{
				if ( hHash != NULL )
				{
					_CryptDestroyHash( hHash );
					hHash = NULL;
				}

				if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
				{
					CreateCNonce( &cnonce, &cnonce_length );

					_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )auth_info->nonce, nonce_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )cnonce, cnonce_length, 0 );

					GlobalFree( HA1 );
					HA1 = NULL;
					HA1_length = 0;

					GetMD5String( &hHash, &HA1, &HA1_length );
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create HA2.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			uri_length = lstrlenA( resource );
			method_length = lstrlenA( method );

			_CryptHashData( hHash, ( BYTE * )method, method_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )resource, uri_length, 0 );

			// auth-int
			// We're not supporting this.
			// We'd have to stream in the HTTP payload body and who knows how large that could be. Forget it!
			if ( auth_info->qop_type == 2 )
			{
				char *entity_body = NULL;
				int entity_body_length = 0;

				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )entity_body, entity_body_length, 0 );
			}

			GetMD5String( &hHash, &HA2, &HA2_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create response.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->nonce, nonce_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );

			if ( auth_info->qop_type != 0 )
			{
				char ncount[ 9 ];
				__snprintf( ncount, 9, "%08x", auth_info->nc );	// Hex must be lowercase.

				if ( cnonce == NULL )
				{
					CreateCNonce( &cnonce, &cnonce_length );
				}

				qop_length = lstrlenA( auth_info->qop );

				_CryptHashData( hHash, ( BYTE * )ncount, 8, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )cnonce, cnonce_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->qop, qop_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			}

			_CryptHashData( hHash, ( BYTE * )HA2, HA2_length, 0 );

			GetMD5String( &hHash, &response, &response_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
	GlobalFree( HA2 );

	if ( auth_info->opaque != NULL )
	{
		opaque_length = lstrlenA( auth_info->opaque );
	}

	// Add up all the value lengths. Use an additional 128 for the value names.
	*auth_key_length = username_length + realm_length + nonce_length + uri_length + response_length + opaque_length + cnonce_length + 128;

	*auth_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( *auth_key_length + 1 ) );

	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "username=\"%s\", ", SAFESTRA( username ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "realm=\"%s\", ", SAFESTRA( auth_info->realm ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "nonce=\"%s\", ", SAFESTRA( auth_info->nonce ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "uri=\"%s\", ", SAFESTRA( resource ) );
	if ( auth_info->algorithm != 0 )
	{
		auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "algorithm=\"%s\", ", ( auth_info->algorithm == 2 ? "MD5-sess" : "MD5" ) );
	}
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "response=\"%s\", ", SAFESTRA( response ) );
	if ( auth_info->opaque != NULL )
	{
		auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "opaque=\"%s\", ", auth_info->opaque );
	}
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "qop=\"%s\", ", SAFESTRA( auth_info->qop ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "nc=\"%08x\", ", auth_info->nc );	// Hex must be lowercase.
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "cnonce=\"%s\"", SAFESTRA( cnonce ) );

	GlobalFree( cnonce );
	GlobalFree( response );

	*auth_key_length = auth_key_offset;
}

void CreateBasicAuthorizationKey( char *username, int username_length, char *password, int password_length, char **auth_key, DWORD *auth_key_length )
{
	*auth_key = NULL;
	*auth_key_length = 0;

	if ( username_length < 0 )
	{
		username_length = lstrlenA( username );
	}
	if ( password_length < 0 )
	{
		password_length = lstrlenA( password );
	}
	int key_length = username_length + password_length + 1;	// Include ":".

	char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( key_length + 1 ) );
	if ( key != NULL )
	{
		_memcpy_s( key, key_length + 1, username, username_length );
		key[ username_length ] = ':';
		_memcpy_s( key + username_length + 1, ( key_length + 1 ) - ( username_length + 1 ), password, password_length );
		key[ key_length ] = 0;	// Sanity.

		_CryptBinaryToStringA( ( BYTE * )key, key_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, auth_key_length );	// auth_key_length WILL include the NULL terminator.

		*auth_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( *auth_key_length ) );
		if ( *auth_key != NULL )
		{
			_CryptBinaryToStringA( ( BYTE * )key, key_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ( LPSTR )*auth_key, auth_key_length );	// auth_key_length DOES NOT include the NULL terminator.
			*( *auth_key + *auth_key_length ) = 0; // Sanity.
		}

		GlobalFree( key );
	}
}

bool VerifyDigestAuthorization( char *username, unsigned long username_length, char *password, unsigned long password_length, char *nonce, unsigned long nonce_length, char *opaque, unsigned long opaque_length, char *method, unsigned long method_length, AUTH_INFO *auth_info )
{
	bool ret = false;

	char *HA1 = NULL;
	DWORD HA1_length = 0;

	char *HA2 = NULL;
	DWORD HA2_length = 0;

	char *response = NULL;
	DWORD response_length = 0;

	int client_response_length = lstrlenA( auth_info->response );
	int client_nonce_length = lstrlenA( auth_info->nonce );
	int client_opaque_length = lstrlenA( auth_info->opaque );

	int cnonce_length = lstrlenA( auth_info->cnonce );
	int realm_length = lstrlenA( auth_info->realm );
	int uri_length = lstrlenA( auth_info->uri );
	int qop_length = lstrlenA( auth_info->qop );

	// We can verify realm, nonce, and opaque to ensure the client responded correctly.
	if ( realm_length != 22 || _StrCmpNA( auth_info->realm, "Authorization Required", 22 != 0 ) )
	{
		return false;
	}

	if ( ( unsigned long )client_nonce_length != nonce_length || _StrCmpNA( auth_info->nonce, nonce, nonce_length ) != 0 )
	{
		return false;
	}

	if ( ( unsigned long )client_opaque_length != opaque_length || _StrCmpNA( auth_info->opaque, opaque, opaque_length ) != 0 )
	{
		return false;
	}

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		// If auth_info->algorithm is not set, then assume it's MD5.

		// Create HA1.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )username, username_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->realm, realm_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )password, password_length, 0 );

			GetMD5String( &hHash, &HA1, &HA1_length );

			// MD5-sess
			if ( auth_info->algorithm == 2 )
			{
				if ( hHash != NULL )
				{
					_CryptDestroyHash( hHash );
					hHash = NULL;
				}

				if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
				{
					_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )nonce, nonce_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )auth_info->cnonce, cnonce_length, 0 );

					GlobalFree( HA1 );
					HA1 = NULL;
					HA1_length = 0;

					GetMD5String( &hHash, &HA1, &HA1_length );
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create HA2.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )method, method_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->uri, uri_length, 0 );

			// auth-int
			// We're not supporting this.
			// We'd have to stream in the HTTP payload body and who knows how large that could be. Forget it!
			if ( auth_info->qop_type == 2 )
			{
				char *entity_body = NULL;
				int entity_body_length = 0;

				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )entity_body, entity_body_length, 0 );
			}

			GetMD5String( &hHash, &HA2, &HA2_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create response.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )nonce, nonce_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );

			if ( auth_info->qop_type != 0 )
			{
				char ncount[ 9 ];
				__snprintf( ncount, 9, "%08x", auth_info->nc );	// Hex must be lowercase.

				_CryptHashData( hHash, ( BYTE * )ncount, 8, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->cnonce, cnonce_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->qop, qop_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			}

			_CryptHashData( hHash, ( BYTE * )HA2, HA2_length, 0 );

			GetMD5String( &hHash, &response, &response_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
	GlobalFree( HA2 );

	if ( response != NULL )
	{
		if ( response_length == ( DWORD )client_response_length && _StrCmpNA( response, auth_info->response, response_length ) == 0 )
		{
			ret = true;
		}

		GlobalFree( response );
	}

	return ret;
}

void AdjustConstructBufferSize( SOCKET_CONTEXT *context, unsigned int offset, char *str, unsigned int str_len = 0 )
{
	if ( context != NULL )
	{
		if ( str != NULL && str_len == 0 )
		{
			str_len = lstrlenA( str );
		}

		while ( ( context->buffer_size - offset ) < str_len )
		{
			context->buffer_size += BUFFER_SIZE;

			char *realloc_buffer = ( char * )GlobalReAlloc( context->buffer, sizeof( char ) * ( context->buffer_size + 1 ), GMEM_MOVEABLE | GMEM_ZEROINIT );
			if ( realloc_buffer != NULL )
			{
				context->buffer = realloc_buffer;

				context->wsabuf.buf = context->buffer;
				context->wsabuf.len = context->buffer_size;
			}
		}
	}
}

void ConstructRequest( SOCKET_CONTEXT *context, bool use_connect )
{
	unsigned int request_length = 0;

	AdjustConstructBufferSize( context, request_length, context->request_info.host );

	char use_http_proxy =  ( cfg_enable_proxy ? 1 : 0 );
	char use_https_proxy = ( cfg_enable_proxy_s ? 1 : 0 );

	PROXY_INFO *pi = NULL;

	if ( context->download_info != NULL &&
	     context->download_info->proxy_info != NULL )
	{
		pi = context->download_info->proxy_info;

		if ( pi->type == 1 )
		{
			use_http_proxy = 2;
		}
		else if ( pi->type == 2 )
		{
			use_https_proxy = 2;
		}
	}

	if ( use_connect )
	{
		request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
				"CONNECT %s:%lu " \
				"HTTP/1.1\r\n" \
				"Host: %s:%lu\r\n",
				context->request_info.host, context->request_info.port, context->request_info.host, context->request_info.port );
	}
	else
	{
		if ( context->download_info != NULL && context->download_info->method == METHOD_POST )
		{
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "POST ", 5 );
			request_length += 5;
		}
		else
		{
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "GET ", 4 );
			request_length += 4;
		}

		if ( use_http_proxy || use_https_proxy )
		{
			if ( context->request_info.protocol == PROTOCOL_HTTPS )
			{
				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "https:", 6 );
				request_length += 6;
			}
			else if ( context->request_info.protocol == PROTOCOL_HTTP )
			{
				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "http:", 5 );
				request_length += 5;
			}

			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "//", 2 );	// Could be protocol-relative.
			request_length += 2;

			int host_length = lstrlenA( context->request_info.host );
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, context->request_info.host, host_length );
			request_length += host_length;

			// Non-standard port for the protocol.
			if ( ( context->request_info.protocol == PROTOCOL_HTTP && context->request_info.port != 80 ) ||
				 ( context->request_info.protocol == PROTOCOL_HTTPS && context->request_info.port != 443 ) )
			{
				request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
						":%lu",
						context->request_info.port );
			}
		}

		AdjustConstructBufferSize( context, request_length, context->request_info.resource );

		// Non-standard port for the protocol.
		if ( ( context->request_info.protocol == PROTOCOL_HTTP && context->request_info.port != 80 ) ||
			 ( context->request_info.protocol == PROTOCOL_HTTPS && context->request_info.port != 443 ) )
		{
			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"%s " \
					"HTTP/1.1\r\n" \
					"Host: %s:%lu\r\n",
					context->request_info.resource,
					context->request_info.host, context->request_info.port );
		}
		else	// No need for the port if it's the default for the protocol.
		{
			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"%s " \
					"HTTP/1.1\r\n" \
					"Host: %s\r\n",
					context->request_info.resource,
					context->request_info.host );
		}

		//_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Accept-Encoding: gzip, deflate\r\n\0", 33 );
		//request_length += 32;

		if ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP )
		{
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Accept-Encoding: gzip\r\n\0", 24 );
			request_length += 23;
		}
		else if ( context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE )
		{
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Accept-Encoding: deflate\r\n\0", 27 );
			request_length += 26;
		}
		else
		{
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Accept-Encoding: identity\r\n\0", 28 );
			request_length += 27;
		}

		// If we're working with a range, then set it.
		if ( context->parts > 1 ||
		   ( context->download_info != NULL && IS_GROUP( context->download_info ) ) ||
		   ( context->header_info.range_info->range_start > 0 &&
			 context->header_info.range_info->range_end > 0 ) )
		{
			/*// The 32-bit version of _snprintf in ntdll.dll on Windows XP doesn't like two %llu.
			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"Range: bytes=%llu-", context->header_info.range_info->range_start );
			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"%llu\r\n", context->header_info.range_info->range_end );*/
			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"Range: bytes=%I64u-%I64u\r\n", context->header_info.range_info->range_start, context->header_info.range_info->range_end );
		}

		/*if ( context->header_info.etag )
		{
			if ( context->download_info != NULL && context->download_info->etag != NULL )
			{
				AdjustConstructBufferSize( context, request_length, context->download_info->etag );

				request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
						"If-Match: %s\r\n", context->download_info->etag );
			}
		}*/

		// Add extra headers.
		if ( context->download_info != NULL && context->download_info->headers != NULL )
		{
			AdjustConstructBufferSize( context, request_length, context->download_info->headers );

			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"%s", context->download_info->headers );
		}

		if ( context->header_info.digest_info != NULL && context->download_info != NULL )
		{
			char *username;
			char *password;

			// The request's username and password (possibly obtained from redirects) must have priority over the download info's username and password.
			if ( context->request_info.auth_info.username != NULL )
			{
				username = context->request_info.auth_info.username;
				password = context->request_info.auth_info.password;
			}
			else
			{
				username = context->download_info->auth_info.username;
				password = context->download_info->auth_info.password;
			}

			if ( context->header_info.digest_info->auth_type == AUTH_TYPE_BASIC )
			{
				char *auth_key = NULL;
				DWORD auth_key_length = 0;
				CreateBasicAuthorizationKey( username, -1, password, -1, &auth_key, &auth_key_length );

				// Even though basic authorization doesn't use a nonce count, we'll use it so we know when to stop retrying the authorization.
				++context->header_info.digest_info->nc;

				if ( auth_key != NULL )
				{
					AdjustConstructBufferSize( context, request_length, NULL, auth_key_length );

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Authorization: Basic ", 21 );
					request_length += 21;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, auth_key, auth_key_length );
					request_length += auth_key_length;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "\r\n\0", 3 );
					request_length += 2;

					GlobalFree( auth_key );
				}
			}
			else if ( context->header_info.digest_info->auth_type == AUTH_TYPE_DIGEST )
			{
				char *auth_key = NULL;
				DWORD auth_key_length = 0;
				CreateDigestAuthorizationKey( username,
											  password,
											  ( use_connect ? "CONNECT" : ( context->download_info->method == METHOD_POST ? "POST" : "GET" ) ),
											  context->request_info.resource,
											  context->header_info.digest_info,
											  &auth_key,
											  &auth_key_length );

				if ( auth_key != NULL )
				{
					AdjustConstructBufferSize( context, request_length, NULL, auth_key_length );

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Authorization: Digest ", 22 );
					request_length += 22;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, auth_key, auth_key_length );
					request_length += auth_key_length;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "\r\n\0", 3 );
					request_length += 2;

					GlobalFree( auth_key );
				}
			}
		}

		if ( context->header_info.cookies != NULL )
		{
			AdjustConstructBufferSize( context, request_length, context->header_info.cookies );

			request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"Cookie: %s\r\n", context->header_info.cookies );
		}

		/*request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
			"Referer: %s%s%s\r\n", context->request_info.protocol == PROTOCOL_HTTPS ? "https://" : "http://", context->request_info.host, context->request_info.resource );*/
	}

	char *proxy_auth_key = NULL;
	unsigned long proxy_auth_key_length = 0;

	char *proxy_auth_username = NULL;
	char *proxy_auth_password = NULL;

	if ( use_http_proxy && context->request_info.protocol == PROTOCOL_HTTP )
	{
		if ( use_http_proxy == 2 && pi != NULL )
		{
			proxy_auth_key = pi->auth_key;
			proxy_auth_key_length = pi->auth_key_length;

			proxy_auth_username = pi->username;
			proxy_auth_password = pi->password;
		}
		else
		{
			proxy_auth_key = g_proxy_auth_key;
			proxy_auth_key_length = g_proxy_auth_key_length;

			proxy_auth_username = g_proxy_auth_username;
			proxy_auth_password = g_proxy_auth_password;
		}
	}
	else if ( use_https_proxy && context->request_info.protocol == PROTOCOL_HTTPS )
	{
		if ( use_https_proxy == 2 && pi != NULL )
		{
			proxy_auth_key = pi->auth_key;
			proxy_auth_key_length = pi->auth_key_length;

			proxy_auth_username = pi->username;
			proxy_auth_password = pi->password;
		}
		else
		{
			proxy_auth_key = g_proxy_auth_key_s;
			proxy_auth_key_length = g_proxy_auth_key_length_s;

			proxy_auth_username = g_proxy_auth_username_s;
			proxy_auth_password = g_proxy_auth_password_s;
		}
	}

	if ( ( use_http_proxy && context->request_info.protocol == PROTOCOL_HTTP ) ||
		 ( use_https_proxy && context->request_info.protocol == PROTOCOL_HTTPS ) )
	{
		if ( context->header_info.proxy_digest_info != NULL )
		{
			if ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_BASIC )
			{
				AdjustConstructBufferSize( context, request_length, NULL, proxy_auth_key_length );

				// Even though basic authorization doesn't use a nonce count, we'll use it so we know when to stop retrying the authorization.
				++context->header_info.proxy_digest_info->nc;

				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Proxy-Authorization: Basic ", 27 );
				request_length += 27;

				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, proxy_auth_key, proxy_auth_key_length );
				request_length += proxy_auth_key_length;

				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "\r\n\0", 3 );
				request_length += 2;
			}
			else if ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_DIGEST )
			{
				char *auth_key = NULL;
				DWORD auth_key_length = 0;
				CreateDigestAuthorizationKey( proxy_auth_username,
											  proxy_auth_password,
											  ( use_connect ? "CONNECT" : ( context->download_info != NULL && context->download_info->method == METHOD_POST ? "POST" : "GET" ) ),
											  context->request_info.resource,
											  context->header_info.proxy_digest_info,
											  &auth_key,
											  &auth_key_length );

				if ( auth_key != NULL )
				{
					AdjustConstructBufferSize( context, request_length, NULL, auth_key_length );

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Proxy-Authorization: Digest ", 28 );
					request_length += 28;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, auth_key, auth_key_length );
					request_length += auth_key_length;

					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "\r\n\0", 3 );
					request_length += 2;

					GlobalFree( auth_key );
				}
			}
		}
	}

	int post_data_length = 0;

	if ( context->download_info != NULL && context->download_info->method == METHOD_POST )
	{
		post_data_length = lstrlenA( context->download_info->data );

		request_length += __snprintf( context->wsabuf.buf + request_length, context->buffer_size - request_length,
					"Content-Length: %lu\r\n", post_data_length );

		//_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Content-Type: application/x-www-form-urlencoded\r\n\0", 50 );
		//request_length += 49;
	}

	if ( context->parts > 1 )
	{
		_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Connection: keep-alive\r\n\r\n\0", 27 );
		request_length += 26;
	}
	else
	{
		_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, "Connection: close\r\n\r\n\0", 22 );
		request_length += 21;
	}

	if ( context->download_info != NULL && context->download_info->method == METHOD_POST )
	{
		AdjustConstructBufferSize( context, request_length, NULL, post_data_length );

		_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, context->download_info->data, post_data_length );
		request_length += post_data_length;
	}

	context->wsabuf.len = min( request_length, context->buffer_size );
}

void ConstructSOCKSRequest( SOCKET_CONTEXT *context, unsigned char request_type )
{
	unsigned int request_length = 0;

	unsigned int dstip = 0x01000000;	// Last byte needs to be non-zero. IP should be invalid so it doesn't get handled.

	char _socks_type = -1;
	bool _resolve_domain_names = false;
	bool _use_authentication = false;

	char *proxy_auth_username = NULL;
	char *proxy_auth_password = NULL;

	if ( context->download_info != NULL &&
	     context->download_info->proxy_info != NULL )
	{
		PROXY_INFO *pi = context->download_info->proxy_info;

		if ( pi->type == 3 )
		{
			_socks_type = SOCKS_TYPE_V4;
			_resolve_domain_names = pi->resolve_domain_names;

			proxy_auth_username = pi->username;
		}
		else if ( pi->type == 4 )
		{
			_socks_type = SOCKS_TYPE_V5;
			_resolve_domain_names = pi->resolve_domain_names;
			_use_authentication = pi->use_authentication;

			proxy_auth_username = pi->username;
			proxy_auth_password = pi->password;
		}
		else
		{
			_socks_type = -1;
		}
	}

	if ( _socks_type == -1 )
	{
		_socks_type = cfg_socks_type;

		if ( cfg_socks_type == SOCKS_TYPE_V4 )
		{
			_resolve_domain_names = cfg_resolve_domain_names_v4a;

			proxy_auth_username = g_proxy_auth_ident_username_socks;
		}
		else if ( cfg_socks_type == SOCKS_TYPE_V5 )
		{
			_resolve_domain_names = cfg_resolve_domain_names;
			_use_authentication = cfg_use_authentication_socks;

			proxy_auth_username = g_proxy_auth_username_socks;
			proxy_auth_password = g_proxy_auth_password_socks;
		}
	}

	if ( _socks_type == SOCKS_TYPE_V4 )	// SOCKS 4
	{
		if ( request_type == 0 )	// Handshake
		{
			context->wsabuf.buf[ request_length++ ] = 0x04;	// Version
			context->wsabuf.buf[ request_length++ ] = 0x01;	// TCP/IP stream connection

			unsigned short nb_port = ( ( context->request_info.port & 0x00FF ) << 8 ) | ( ( context->request_info.port & 0xFF00 ) >> 8 );
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &nb_port, sizeof( unsigned short ) );
			request_length += sizeof( unsigned short );

			if ( _resolve_domain_names )
			{
				// The last byte needs to be non-zero for the domain resolve to occur: 0.0.0.x 
				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
				request_length += sizeof( unsigned int );
			}
			else
			{
				if ( context->proxy_address_info != NULL )
				{
					struct sockaddr_in *ipv4_addr = ( struct sockaddr_in * )context->proxy_address_info->ai_addr;
					if ( ipv4_addr != NULL )
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &ipv4_addr->sin_addr, sizeof( ipv4_addr->sin_addr ) );
						request_length += sizeof( ipv4_addr->sin_addr );
					}
					else
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
						request_length += sizeof( unsigned int );
					}
				}
				else
				{
					_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
					request_length += sizeof( unsigned int );
				}
			}

			unsigned char username_length = ( unsigned char )lstrlenA( proxy_auth_username ) + 1;	// Include the NULL character.

			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, proxy_auth_username, username_length );
			request_length += username_length;

			if ( _resolve_domain_names )
			{
				int host_length = ( int )lstrlenA( context->request_info.host ) + 1;	// Include the NULL character.

				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, context->request_info.host, host_length );
				request_length += host_length;
			}
		}
	}
	else if ( _socks_type == SOCKS_TYPE_V5 )	// SOCKS 5
	{
		if ( request_type == 0 )		// Authentication request.
		{
			context->wsabuf.buf[ request_length++ ] = 0x05;	// Version

			if ( _use_authentication )
			{
				context->wsabuf.buf[ request_length++ ] = 0x02;	// Number of methods
			}
			else
			{
				context->wsabuf.buf[ request_length++ ] = 0x01;	// Number of methods
			}

			context->wsabuf.buf[ request_length++ ] = 0x00;	// Method: None

			if ( _use_authentication )
			{
				context->wsabuf.buf[ request_length++ ] = 0x02;	// Method: Username and Password
			}
		}
		else if ( request_type == 1 )	// Connection request.
		{
			context->wsabuf.buf[ request_length++ ] = 0x05;	// Version
			context->wsabuf.buf[ request_length++ ] = 0x01;	// TCP/IP stream connection
			context->wsabuf.buf[ request_length++ ] = 0x00;	// Reserved

			if ( _resolve_domain_names )
			{
				context->wsabuf.buf[ request_length++ ] = 0x03;	// Domain name.

				unsigned char host_length = ( unsigned char )lstrlenA( context->request_info.host );

				// The proxy doesn't like the brackets in the host name, so we'll exclude them in the request.
				bool is_ipv6_address = ( host_length >= 2 &&
										 context->request_info.host != NULL &&
										 context->request_info.host[ 0 ] == '[' &&
										 context->request_info.host[ host_length - 1 ] == ']' ? true : false );

				if ( is_ipv6_address )
				{
					host_length -= 2;
				}

				context->wsabuf.buf[ request_length++ ] = host_length;

				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, context->request_info.host + ( is_ipv6_address ? 1 : 0 ), host_length );

				request_length += host_length;

				unsigned short nb_port = ( ( context->request_info.port & 0x00FF ) << 8 ) | ( ( context->request_info.port & 0xFF00 ) >> 8 );
				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &nb_port, sizeof( unsigned short ) );
				request_length += sizeof( unsigned short );
			}
			else if ( context->proxy_address_info != NULL )
			{
				bool use_ipv6 = ( context->proxy_address_info->ai_family == AF_INET6 ? true : false );

				context->wsabuf.buf[ request_length++ ] = ( use_ipv6 ? 0x04 : 0x01 );	// IPV6 or IPV4.

				if ( use_ipv6 )	// IPV6
				{
					struct sockaddr_in6 *ipv6_addr = ( struct sockaddr_in6 * )context->proxy_address_info->ai_addr;
					if ( ipv6_addr != NULL )
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &ipv6_addr->sin6_addr, sizeof( ipv6_addr->sin6_addr ) );
						request_length += sizeof( ipv6_addr->sin6_addr );

						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &ipv6_addr->sin6_port, sizeof( ipv6_addr->sin6_port ) );
						request_length += sizeof( ipv6_addr->sin6_port );
					}
					else
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
						request_length += sizeof( unsigned int );
					}
				}
				else
				{
					struct sockaddr_in *ipv4_addr = ( struct sockaddr_in * )context->proxy_address_info->ai_addr;
					if ( ipv4_addr != NULL )
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &ipv4_addr->sin_addr, sizeof( ipv4_addr->sin_addr ) );
						request_length += sizeof( ipv4_addr->sin_addr );

						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &ipv4_addr->sin_port, sizeof( ipv4_addr->sin_port ) );
						request_length += sizeof( ipv4_addr->sin_port );
					}
					else
					{
						_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
						request_length += sizeof( unsigned int );
					}
				}
			}
			else
			{
				_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, &dstip, sizeof( unsigned int ) );
				request_length += sizeof( unsigned int );
			}
		}
		else if ( request_type == 2 )
		{
			unsigned char username_length = ( proxy_auth_username != NULL ? ( unsigned char )lstrlenA( proxy_auth_username ) : 0 );
			unsigned char password_length = ( proxy_auth_password != NULL ? ( unsigned char )lstrlenA( proxy_auth_password ) : 0 );

			context->wsabuf.buf[ request_length++ ] = 0x01;	// Username/Password version
			context->wsabuf.buf[ request_length++ ] = username_length;
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, proxy_auth_username, username_length );
			request_length += username_length;
			context->wsabuf.buf[ request_length++ ] = password_length;
			_memcpy_s( context->wsabuf.buf + request_length, context->buffer_size - request_length, proxy_auth_password, password_length );
			request_length += password_length;
		}
	}

	context->wsabuf.len = request_length;
}

#ifdef ENABLE_LOGGING

#include "lite_dlls.h"

bool g_can_log = false;
unsigned int g_log_filter = 0x00000000;

HMODULE hModule_ntdll_logging = NULL;
HMODULE hModule_ws2_32_logging = NULL;

CRITICAL_SECTION logging_cs;

HANDLE hFile_log = INVALID_HANDLE_VALUE;

char *g_log_buffer = NULL;
unsigned int g_log_buffer_offset = 0;

// ntdll
//p_vsnwprintf			__vsnwprintf;
p_vsnprintf				__vsnprintf;
pRtlGetNtVersionNumbers	_RtlGetNtVersionNumbers;

// ws2_32
pGetNameInfoW	_GetNameInfoW;
pgetpeername	_getpeername;

//

char *g_size_prefix_a[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };

unsigned int FormatSizesA( char *buffer, unsigned int buffer_size, unsigned char toggle_type, unsigned long long data_size )
{
	unsigned int length;

	double divisor;

	if ( toggle_type == SIZE_FORMAT_AUTO )
	{
		if ( data_size >= ( 1ULL << 60 ) )
		{
			toggle_type = SIZE_FORMAT_EXABYTE;	// Exabyte
		}
		else if ( data_size >= ( 1ULL << 50 ) )
		{
			toggle_type = SIZE_FORMAT_PETABYTE;	// Petabyte
		}
		else if ( data_size >= ( 1ULL << 40 ) )
		{
			toggle_type = SIZE_FORMAT_TERABYTE;	// Terabyte
		}
		else if ( data_size >= ( 1 << 30 ) )
		{
			toggle_type = SIZE_FORMAT_GIGABYTE;	// Gigabyte
		}
		else if ( data_size >= ( 1 << 20 ) )
		{
			toggle_type = SIZE_FORMAT_MEGABYTE;	// Megabyte
		}
		else if ( data_size >= ( 1 << 10 ) )
		{
			toggle_type = SIZE_FORMAT_KILOBYTE;	// Kilobyte
		}
		else
		{
			toggle_type = SIZE_FORMAT_BYTE;		// Byte
		}
	}

	if ( toggle_type != SIZE_FORMAT_BYTE )
	{
		divisor = ( double )( 1ULL << ( toggle_type * 10 ) );

		unsigned long long i_percentage;
#ifdef _WIN64
		i_percentage = ( unsigned long long )( 100.0 * ( ( double )data_size / divisor ) );
#else
		// This leaves us with an integer in which the last digit will represent the decimal value.
		double f_percentage = 100.0 * ( ( double )data_size / divisor );
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
#endif
		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snprintf( buffer, buffer_size, "%I64u.%02lu %s", i_percentage, remainder, g_size_prefix_a[ toggle_type ] );
	}
	else
	{
		length = __snprintf( buffer, buffer_size, "%I64u %s", data_size, g_size_prefix_a[ toggle_type ] );
	}

	return length;
}

void GetDownloadStatus( char *buf, unsigned short buf_size, unsigned int status )
{
	unsigned short offset = 0;
	unsigned int mask = 1;

	char *val;

	for ( unsigned int i = 0; i < 20 && status != 0; ++i )
	{
		val = NULL;

		switch ( status & mask )
		{
			//case STATUS_NONE:
			case STATUS_CONNECTING:					{ val = "Connecting"; } break;
			case STATUS_DOWNLOADING:				{ val = "Downloading"; } break;
			case STATUS_PAUSED:						{ val = "Paused"; } break;
			case STATUS_QUEUED:						{ val = "Queued"; } break;
			case STATUS_COMPLETED:					{ val = "Completed"; } break;
			case STATUS_STOPPED:					{ val = "Stopped"; } break;
			case STATUS_TIMED_OUT:					{ val = "Timed Out"; } break;
			case STATUS_FAILED:						{ val = "Failed"; } break;
			case STATUS_RESTART:					{ val = "Restart"; } break;
			case STATUS_REMOVE:						{ val = "Remove"; } break;
			case STATUS_DELETE:						{ val = "Delete"; } break;
			case STATUS_FILE_IO_ERROR:				{ val = "File IO Error"; } break;
			case STATUS_SKIPPED:					{ val = "Skipped"; } break;
			case STATUS_AUTH_REQUIRED:				{ val = "Authorization Required"; } break;
			case STATUS_PROXY_AUTH_REQUIRED:		{ val = "Proxy Authentication Required"; } break;
			case STATUS_UPDATING:					{ val = "Updating"; } break;
			case STATUS_ALLOCATING_FILE:			{ val = "Allocating File"; } break;
			case STATUS_MOVING_FILE:				{ val = "Moving File"; } break;
			case STATUS_INPUT_REQUIRED:				{ val = "Input Required"; } break;
			case STATUS_INSUFFICIENT_DISK_SPACE:	{ val = "Insufficient Disk Space"; } break;
		}

		status &= ~mask;

		if ( val != NULL )
		{
			if ( offset > 0 )
			{
				buf[ offset++ ] = ',';
				buf[ offset++ ] = ' ';
			}

			unsigned char val_length = ( unsigned char )lstrlenA( val );
			_memcpy_s( buf + offset, buf_size - offset, val, val_length );

			offset += val_length;

			buf[ offset ] = 0;	// Sanity.
		}

		mask <<= 1;
	}
}

void GenericLogEntry( DOWNLOAD_INFO *di, unsigned int type, char *msg )
{
	if ( di != NULL && msg != NULL )
	{
		wchar_t *l_file_path;
		wchar_t t_l_file_path[ MAX_PATH ];
		bool is_temp = false;
		if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE )
		{
			l_file_path = L"Simulated";
		}
		else
		{
			if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
			else { GetDownloadFilePath( di, t_l_file_path ); }
			l_file_path = t_l_file_path;
		}
		WriteLog( type, "%s: %s%S | %s%S", msg, ( IS_GROUP( di ) ? "group | " : "" ), di->url, ( is_temp ? "temp | " : "" ), l_file_path );
	}
}

bool InitLogging()
{
	hModule_ntdll_logging = LoadLibraryDEMW( L"ntdll.dll" );
	hModule_ws2_32_logging = LoadLibraryDEMW( L"ws2_32.dll" );

	if ( hModule_ntdll_logging == NULL || hModule_ws2_32_logging == NULL )
	{
		return false;
	}

	// ntdll
	//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll_logging, ( void ** )&__vsnwprintf, "_vsnwprintf" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll_logging, ( void ** )&__vsnprintf, "_vsnprintf" ) )

	// Undocumented
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll_logging, ( void ** )&_RtlGetNtVersionNumbers, "RtlGetNtVersionNumbers" ) )

	// ws2_32
	//
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32_logging, ( void ** )&_GetNameInfoW, "GetNameInfoW" ) )
	VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ws2_32_logging, ( void ** )&_getpeername, "getpeername" ) )

	//

	InitializeCriticalSection( &logging_cs );

	g_log_buffer = ( char * )GlobalAlloc( GMEM_FIXED, ( LOG_BUFFER_SIZE * sizeof( char ) ) );

	g_can_log = true;

	return true;
}

bool UnInitLogging()
{
	BOOL ret = TRUE;

	if ( g_can_log )
	{
		if ( g_log_buffer != NULL )
		{
			GlobalFree( g_log_buffer );
			g_log_buffer = NULL;
		}

		g_log_buffer_offset = 0;

		DeleteCriticalSection( &logging_cs );
	}

	if ( hModule_ntdll_logging != NULL )
	{
		ret &= FreeLibrary( hModule_ntdll_logging );
	}

	if ( hModule_ws2_32_logging != NULL )
	{
		ret &= FreeLibrary( hModule_ws2_32_logging );
	}

	return ( ret != FALSE ? true : false );
}

void OpenLog( wchar_t *file_path, unsigned int log_filter )
{
	if ( g_can_log )
	{
		EnterCriticalSection( &logging_cs );

		if ( file_path != NULL && hFile_log == INVALID_HANDLE_VALUE )
		{
			hFile_log = CreateFile( file_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

			if ( hFile_log != INVALID_HANDLE_VALUE )
			{
				// Write the UTF-8 BOM
				if ( GetLastError() != ERROR_ALREADY_EXISTS )
				{
					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, "\xEF\xBB\xBF", 3 );
					g_log_buffer_offset += 3;
				}
				else
				{
					LARGE_INTEGER li;
					li.QuadPart = 0;

					SetFilePointerEx( hFile_log, li, NULL, FILE_END );
				}

				g_log_filter = log_filter;

				if ( g_log_filter & LOG_INFO_MISC )
				{
					SYSTEMTIME st;

					GetSystemTime( &st );

					g_log_buffer_offset += __snprintf( LOG_BUFFER, LOG_BUFFER_OFFSET,
											  "--------------------------------------------------\r\n" \
											  "Logging started at: %04d-%02d-%02d %02d:%02d:%02d.%03d UTC\r\n" \
											  "HTTP Downloader version: %lu.%lu.%lu.%lu (%u-bit)\r\n",
											  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
											  CURRENT_VERSION_A, CURRENT_VERSION_B, CURRENT_VERSION_C, CURRENT_VERSION_D,
					#ifdef _WIN64
											  64
					#else
											  32
					#endif
											  );

					///////

					SYSTEM_INFO systemInfo;
					GetSystemInfo( &systemInfo );
					char *arch;

					#define PROCESSOR_ARCHITECTURE_ARM64 12

					switch ( systemInfo.wProcessorArchitecture )
					{
						case PROCESSOR_ARCHITECTURE_AMD64: { arch = "x64"; } break;
						case PROCESSOR_ARCHITECTURE_ARM: { arch = "ARM"; } break;
						case PROCESSOR_ARCHITECTURE_ARM64: { arch = "ARM64"; } break;
						case PROCESSOR_ARCHITECTURE_IA64: { arch = "Intel Itanium"; } break;
						case PROCESSOR_ARCHITECTURE_INTEL: { arch = "x86"; } break;
						case PROCESSOR_ARCHITECTURE_UNKNOWN:
						default: { arch = "Unknown"; } break;
					}

					DWORD major, minor, buildNumber = 0;
					_RtlGetNtVersionNumbers( &major, &minor, &buildNumber );
					buildNumber &= ~0xF0000000;

					g_log_buffer_offset += __snprintf( LOG_BUFFER, LOG_BUFFER_OFFSET,
								"Logical processors: %lu\r\n" \
								"Architecture: %s\r\n" \
								"Windows build: %lu.%lu.%lu\r\n",
								systemInfo.dwNumberOfProcessors,
								arch,
								major, minor, buildNumber );

					///////

					MEMORYSTATUSEX mstatus;
					mstatus.dwLength = sizeof( MEMORYSTATUSEX );
					GlobalMemoryStatusEx( &mstatus );

					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, "Memory usage: ", 14 );
					g_log_buffer_offset += 14;

					g_log_buffer_offset += FormatSizesA( LOG_BUFFER, LOG_BUFFER_OFFSET, SIZE_FORMAT_AUTO, mstatus.ullTotalPhys - mstatus.ullAvailPhys );

					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, " / ", 3 );
					g_log_buffer_offset += 3;

					g_log_buffer_offset += FormatSizesA( LOG_BUFFER, LOG_BUFFER_OFFSET, SIZE_FORMAT_AUTO, mstatus.ullTotalPhys );

					//

					ULARGE_INTEGER free_bytes, total_bytes;

					GetDiskFreeSpaceExW( NULL, &free_bytes, &total_bytes, NULL );

					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, "\r\nDisk usage: ", 14 );
					g_log_buffer_offset += 14;

					g_log_buffer_offset += FormatSizesA( LOG_BUFFER, LOG_BUFFER_OFFSET, SIZE_FORMAT_AUTO, total_bytes.QuadPart - free_bytes.QuadPart );

					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, " / ", 3 );
					g_log_buffer_offset += 3;

					g_log_buffer_offset += FormatSizesA( LOG_BUFFER, LOG_BUFFER_OFFSET, SIZE_FORMAT_AUTO, total_bytes.QuadPart );

					_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, "\r\n--------------------------------------------------\r\n", 54 );
					g_log_buffer_offset += 54;

					///////
				}

				if ( g_log_buffer_offset > 0 )
				{
					DWORD written = 0;
					WriteFile( hFile_log, g_log_buffer, g_log_buffer_offset, &written, NULL );
					g_log_buffer_offset = 0;
				}
			}
		}

		LeaveCriticalSection( &logging_cs );
	}
}

void CloseLog()
{
	if ( g_can_log )
	{
		EnterCriticalSection( &logging_cs );

		if ( hFile_log != INVALID_HANDLE_VALUE )
		{
			CloseHandle( hFile_log );
			hFile_log = INVALID_HANDLE_VALUE;
		}

		LeaveCriticalSection( &logging_cs );
	}
}

void WriteLog( unsigned int type, const char *format, ... )
{
	if ( g_can_log && ( type & g_log_filter ) )
	{
		EnterCriticalSection( &logging_cs );

		if ( hFile_log != INVALID_HANDLE_VALUE )
		{
			va_list arglist;

			SYSTEMTIME st;

			GetSystemTime( &st );

			char *str_type;

			if ( type & ( LOG_INFO_MISC | LOG_INFO_ACTION | LOG_INFO_CON_STATE ) )
			{
				str_type = "INFO";
			}
			else if ( type & LOG_WARNING )
			{
				str_type = "WARNING";
			}
			else if ( type & LOG_ERROR )
			{
				str_type = "ERROR";
			}
			else
			{
				str_type = "";
			}

			g_log_buffer_offset += __snprintf( LOG_BUFFER, LOG_BUFFER_OFFSET, "%04d-%02d-%02d %02d:%02d:%02d.%03d UTC | %s | ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, str_type );

			va_start( arglist, format );

			g_log_buffer_offset += __vsnprintf( LOG_BUFFER, LOG_BUFFER_OFFSET, format, arglist );

			_memcpy_s( LOG_BUFFER, LOG_BUFFER_OFFSET, "\r\n", 2 );

			g_log_buffer_offset += 2;

			va_end( arglist );

			///////

			DWORD written = 0;
			WriteFile( hFile_log, g_log_buffer, g_log_buffer_offset, &written, NULL );
			g_log_buffer_offset = 0;
		}

		LeaveCriticalSection( &logging_cs );
	}
}

#endif
