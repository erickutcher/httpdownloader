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

#ifndef _GLOBALS_H
#define _GLOBALS_H

// Pretty window.
#pragma comment( linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <process.h>

#include "lite_user32.h"
#include "lite_shell32.h"
#include "lite_ntdll.h"

#include "dllrbt.h"

#include "resource.h"

#define MIN_WIDTH			800
#define MIN_HEIGHT			600

#define DW_WIDTH			48
#define DW_HEIGHT			48

#define SPLITTER_WIDTH		3
#define SPLITTER_POS_X		140

#define SNAP_WIDTH			10		// The minimum distance at which our windows will attach together.

#define WM_DPICHANGED		0x02E0
#define WM_DPICHANGED_BEFOREPARENT	0x02E2
#define WM_DPICHANGED_AFTERPARENT	0x02E3
//#define WM_GETDPISCALEDSIZE	0x02E4


#define WM_DESTROY_ALT		WM_APP		// Allows non-window threads to call DestroyWindow.
#define WM_PROPAGATE		WM_APP + 1
#define WM_CHANGE_CURSOR	WM_APP + 2	// Updates the window cursor.
#define WM_FILTER_TEXT		WM_APP + 3

#define WM_TRAY_NOTIFY		WM_APP + 4
#define WM_EXIT				WM_APP + 5
#define WM_ALERT			WM_APP + 6

#define WM_RESET_PROGRESS	WM_APP + 7

#define WM_UPDATE_CATEGORY	WM_APP + 8
#define WM_GET_DPI			WM_APP + 9
#define WM_OPTIONS_CHANGED	WM_APP + 10
#define WM_SAVE_OPTIONS		WM_APP + 11

#define WM_PEER_CONNECTED	WM_APP + 12

#define FILETIME_TICKS_PER_SECOND	10000000LL

#define NUM_COLUMNS			17

#define COLUMN_NUM					0
#define COLUMN_ACTIVE_PARTS			1
#define COLUMN_CATEGORY				2
#define COLUMN_COMMENTS				3
#define COLUMN_DATE_AND_TIME_ADDED	4
#define COLUMN_DOWNLOAD_DIRECTORY	5
#define COLUMN_DOWNLOAD_SPEED		6
#define COLUMN_DOWNLOAD_SPEED_LIMIT	7
#define COLUMN_DOWNLOADED			8
#define COLUMN_FILE_SIZE			9
#define COLUMN_FILE_TYPE			10
#define COLUMN_FILENAME				11
#define COLUMN_PROGRESS				12
#define COLUMN_SSL_TLS_VERSION		13
#define COLUMN_TIME_ELAPSED			14
#define COLUMN_TIME_REMAINING		15
#define COLUMN_URL					16

#define NUM_COLORS			80
#define TD_NUM_COLORS		12

#define MAX_DOMAIN_LENGTH	253

#define _wcsicmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiW( a, b ) )
#define _stricmp_s( a, b ) ( ( a == NULL && b == NULL ) ? 0 : ( a != NULL && b == NULL ) ? 1 : ( a == NULL && b != NULL ) ? -1 : lstrcmpiA( a, b ) )

#define SAFESTRA( s ) ( s != NULL ? s : "" )
#define SAFESTR2A( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : "" ) )

#define SAFESTRW( s ) ( s != NULL ? s : L"" )
#define SAFESTR2W( s1, s2 ) ( s1 != NULL ? s1 : ( s2 != NULL ? s2 : L"" ) )

#define is_close( a, b ) ( _abs( ( a ) - ( b ) ) < SNAP_WIDTH )

#define is_digit_a( c ) ( c - '0' + 0U <= 9U )
#define is_digit_w( c ) ( c - L'0' + 0U <= 9U )

#define GET_X_LPARAM( lp )	( ( int )( short )LOWORD( lp ) )
#define GET_Y_LPARAM( lp )	( ( int )( short )HIWORD( lp ) )

#define PROGRAM_CAPTION		L"HTTP Downloader"
#define PROGRAM_CAPTION_A	"HTTP Downloader"

#define HOME_PAGE			L"https://erickutcher.github.io/#HTTP_Downloader"
#define CHANGELOG			L"https://raw.githubusercontent.com/erickutcher/httpdownloader/master/HTTP_Downloader/changelog.txt"

#define UPDATE_CHECK_URL	"https://raw.githubusercontent.com/erickutcher/httpdownloader/master/HTTP_Downloader/version.txt"

#define CURRENT_VERSION_A	1
#define CURRENT_VERSION_B	0
#define CURRENT_VERSION_C	6
#define CURRENT_VERSION_D	9

#define CURRENT_VERSION		( ( CURRENT_VERSION_A << 24 ) | \
							  ( CURRENT_VERSION_B << 16 ) | \
							  ( CURRENT_VERSION_C << 8 )  | \
							  ( CURRENT_VERSION_D ) )

//#define IS_BETA

#ifdef IS_BETA
#define BETA_VERSION		0
#define UPDATE_CHECK_URL_BETA	"https://raw.githubusercontent.com/erickutcher/httpdownloader/master/HTTP_Downloader/version_beta.txt"
#endif

#define SIZE_FORMAT_BYTE		0
#define SIZE_FORMAT_KILOBYTE	1
#define SIZE_FORMAT_MEGABYTE	2
#define SIZE_FORMAT_GIGABYTE	3
#define SIZE_FORMAT_TERABYTE	4
#define SIZE_FORMAT_PETABYTE	5
#define SIZE_FORMAT_EXABYTE		6
#define SIZE_FORMAT_AUTO		7

#define SHUTDOWN_ACTION_NONE				0
#define SHUTDOWN_ACTION_EXIT_PROGRAM		1
#define SHUTDOWN_ACTION_LOG_OFF				2
#define SHUTDOWN_ACTION_LOCK				3
#define SHUTDOWN_ACTION_RESTART				4
#define SHUTDOWN_ACTION_SLEEP				5
#define SHUTDOWN_ACTION_HIBERNATE			6
#define SHUTDOWN_ACTION_SHUT_DOWN			7
#define SHUTDOWN_ACTION_HYBRID_SHUT_DOWN	8

#define DRAG_AND_DROP_ACTION_NONE					0
#define DRAG_AND_DROP_ACTION_DOWNLOAD_IMMEDIATELY	1
#define DRAG_AND_DROP_ACTION_ADD_IN_STOPPED_STATE	2
#define DRAG_AND_DROP_ACTION_VERIFY					3

struct SORT_INFO
{
	HWND hWnd;
	int column;
	unsigned char direction;
};

struct ICON_INFO
{
	wchar_t *file_extension;
	HICON icon;
	unsigned int count;
};

struct SEARCH_INFO
{
	wchar_t *text;
	unsigned char type;			// 0 = Filename, 1 = URL
	unsigned char search_flag;	// 0x00 = None, 0x01 = Match case, 0x02 = Match whole word, 0x04 = Regular expression.
	bool search_all;
};

struct FILTER_INFO
{
	wchar_t *text;
	wchar_t *filter;
};

union QFILETIME
{
	FILETIME ft;
	ULONGLONG ull;
};

struct CL_ARGS
{
	unsigned long long download_speed_limit;
	wchar_t *category;
	wchar_t *download_directory;
	wchar_t *download_history_file;
	wchar_t *url_list_file;
	wchar_t *urls;
	wchar_t *comments;
	wchar_t *cookies;
	wchar_t *headers;
	wchar_t *data;
	wchar_t *username;
	wchar_t *password;
	wchar_t *proxy_hostname;
	wchar_t *proxy_username;
	wchar_t *proxy_password;
	unsigned long proxy_ip_address;
	unsigned int download_operations;
	int category_length;
	int download_directory_length;
	int download_history_file_length;
	int url_list_file_length;
	int urls_length;
	int comments_length;
	int cookies_length;
	int headers_length;
	int data_length;
	int username_length;
	int password_length;
	int proxy_hostname_length;
	int proxy_username_length;
	int proxy_password_length;
	unsigned short proxy_port;
	unsigned char parts;
	unsigned char download_immediately;
	unsigned char proxy_type;
	char ssl_version;
	bool use_clipboard;
	bool proxy_resolve_domain_names;	// v4a or v5 based on proxy_type
	bool use_download_speed_limit;
	bool use_download_directory;
	bool use_parts;
};

struct FONT_SETTINGS
{
	LOGFONT lf;
	HFONT font;
	COLORREF font_color;
};

// These are all variables that are shared among the separate .cpp files.

// Object handles.
extern HWND g_hWnd_main;				// Handle to our main window.
extern HWND g_hWnd_add_urls;
extern HWND g_hWnd_options;
extern HWND g_hWnd_update_download;
extern HWND g_hWnd_search;
extern HWND g_hWnd_download_speed_limit;
extern HWND g_hWnd_add_category;
extern HWND g_hWnd_url_drop_window;

extern HWND g_hWnd_check_for_updates;

extern HWND g_hWnd_fingerprint_prompt;

extern HWND g_hWnd_site_manager;
extern HWND g_hWnd_site_list;

extern HWND g_hWnd_sftp_fps_host_list;
extern HWND g_hWnd_sftp_keys_host_list;

extern HWND g_hWnd_toolbar;
extern HWND g_hWnd_status;
extern HWND g_hWnd_categories;
extern HWND g_hWnd_tlv_files;

extern HWND g_hWnd_active;				// Handle to the active window. Used to handle tab stops.

extern LOGFONT g_default_log_font;
extern HFONT g_hFont;

extern UINT current_dpi_main;

extern dllrbt_tree *g_icon_handles;

extern dllrbt_tree *g_site_info;
extern dllrbt_tree *g_sftp_fps_host_info;
extern dllrbt_tree *g_sftp_keys_host_info;
extern dllrbt_tree *g_category_info;

extern bool	g_can_fast_allocate;			// Prevent the pre-allocation from zeroing the file.

extern int g_default_row_height;

extern bool g_skip_list_draw;

extern int g_file_size_cmb_ret;			// Message box prompt for large files sizes.

extern HANDLE worker_semaphore;			// Blocks shutdown while a worker thread is active.
extern bool in_worker_thread;
extern bool kill_worker_thread_flag;	// Allow for a clean shutdown.

extern bool g_download_history_changed;

extern HANDLE g_downloader_ready_semaphore;

extern CRITICAL_SECTION worker_cs;				// Worker thread critical section.

extern CRITICAL_SECTION session_totals_cs;

extern CRITICAL_SECTION icon_cache_cs;

extern wchar_t *g_base_directory;
extern unsigned int g_base_directory_length;

extern wchar_t *g_program_directory;
extern unsigned int g_program_directory_length;

extern unsigned int g_default_download_directory_length;
extern unsigned int g_temp_download_directory_length;

extern bool g_allow_rename;

extern UINT CF_HTML;			// Clipboard format.
extern UINT CF_TREELISTVIEW;	// Clipboard format.

extern unsigned long long g_session_total_downloaded;
extern unsigned long long g_session_downloaded_speed;

extern unsigned long long g_session_last_total_downloaded;
extern unsigned long long g_session_last_downloaded_speed;

// Settings

extern int cfg_pos_x;
extern int cfg_pos_y;
extern int cfg_width;
extern int cfg_height;

extern char cfg_min_max;

extern int cfg_splitter_pos_x;

extern int cfg_column_width1;
extern int cfg_column_width2;
extern int cfg_column_width3;
extern int cfg_column_width4;
extern int cfg_column_width5;
extern int cfg_column_width6;
extern int cfg_column_width7;
extern int cfg_column_width8;
extern int cfg_column_width9;
extern int cfg_column_width10;
extern int cfg_column_width11;
extern int cfg_column_width12;
extern int cfg_column_width13;
extern int cfg_column_width14;
extern int cfg_column_width15;
extern int cfg_column_width16;
extern int cfg_column_width17;

extern char cfg_column_order1;
extern char cfg_column_order2;
extern char cfg_column_order3;
extern char cfg_column_order4;
extern char cfg_column_order5;
extern char cfg_column_order6;
extern char cfg_column_order7;
extern char cfg_column_order8;
extern char cfg_column_order9;
extern char cfg_column_order10;
extern char cfg_column_order11;
extern char cfg_column_order12;
extern char cfg_column_order13;
extern char cfg_column_order14;
extern char cfg_column_order15;
extern char cfg_column_order16;
extern char cfg_column_order17;

extern bool cfg_show_toolbar;
extern bool cfg_show_categories;
extern bool cfg_show_column_headers;
extern bool cfg_show_status_bar;

extern unsigned char cfg_t_downloaded;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
extern unsigned char cfg_t_file_size;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
extern unsigned char cfg_t_down_speed;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
extern unsigned char cfg_t_speed_limit;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.

extern unsigned char cfg_t_status_downloaded;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
extern unsigned char cfg_t_status_down_speed;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.
extern unsigned char cfg_t_status_speed_limit;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, etc.

extern unsigned long long cfg_total_downloaded;

extern int cfg_drop_pos_x;	// URL drop window.
extern int cfg_drop_pos_y;	// URL drop window.

extern bool cfg_tray_icon;
extern bool cfg_minimize_to_tray;
extern bool cfg_close_to_tray;
extern bool cfg_start_in_tray;
extern bool cfg_show_notification;

extern bool cfg_show_remote_connection_notification;

extern bool cfg_always_on_top;
extern bool cfg_check_for_updates;
extern bool cfg_enable_download_history;
extern bool cfg_enable_quick_allocation;
extern bool cfg_enable_sparse_file_allocation;
extern bool cfg_set_filetime;
extern bool cfg_update_redirected;
extern bool cfg_apply_initial_proxy;
extern bool cfg_download_non_200_206;
extern bool cfg_move_to_trash;
extern bool cfg_override_list_prompts;
extern bool cfg_use_one_instance;
extern bool cfg_enable_drop_window;
extern bool cfg_prevent_standby;
extern bool cfg_category_move;

extern unsigned char cfg_drag_and_drop_action;

extern unsigned char cfg_shutdown_action;
extern unsigned char g_shutdown_action;

extern bool cfg_play_sound;
extern wchar_t *cfg_sound_file_path;

extern bool cfg_play_sound_fail;
extern wchar_t *cfg_sound_fail_file_path;

extern unsigned long cfg_thread_count;
extern unsigned long g_max_threads;

extern unsigned char cfg_max_downloads;

extern unsigned char cfg_retry_downloads_count;
extern unsigned char cfg_retry_parts_count;

extern unsigned char cfg_default_ssl_version;
extern unsigned char cfg_default_download_parts;

extern bool cfg_reallocate_parts;
extern unsigned long long cfg_reallocate_threshold_size;

extern unsigned char cfg_max_redirects;

extern unsigned long long cfg_default_speed_limit;

extern wchar_t *cfg_default_download_directory;

// FTP

extern unsigned char cfg_ftp_mode_type;
extern bool cfg_ftp_enable_fallback_mode;
extern unsigned char cfg_ftp_address_type;
extern wchar_t *cfg_ftp_hostname;
extern unsigned long cfg_ftp_ip_address;
extern unsigned short cfg_ftp_port_start;
extern unsigned short cfg_ftp_port_end;

extern bool cfg_ftp_send_keep_alive;

// SFTP

extern bool cfg_sftp_enable_compression;
extern bool cfg_sftp_attempt_gssapi_authentication;
extern bool cfg_sftp_attempt_gssapi_key_exchange;
extern int cfg_sftp_keep_alive_time;
extern int cfg_sftp_rekey_time;
extern int cfg_sftp_gss_rekey_time;
extern unsigned long cfg_sftp_rekey_data_limit;

#define KEX_ALGORITHM_COUNT		5
#define HOST_KEY_COUNT			4
#define ENCRYPTION_CIPHER_COUNT	6

extern unsigned char cfg_priority_kex_algorithm[ KEX_ALGORITHM_COUNT ];
extern unsigned char cfg_priority_host_key[ HOST_KEY_COUNT ];
extern unsigned char cfg_priority_encryption_cipher[ ENCRYPTION_CIPHER_COUNT ];

// Server

extern bool cfg_enable_server;
extern unsigned char cfg_server_address_type;
extern unsigned long cfg_server_ip_address;
extern wchar_t *cfg_server_hostname;
extern unsigned short cfg_server_port;

extern unsigned char cfg_server_ssl_version;

extern bool cfg_server_enable_ssl;

extern unsigned char cfg_certificate_type;

extern wchar_t *cfg_certificate_pkcs_file_name;
extern wchar_t *cfg_certificate_pkcs_password;

extern wchar_t *cfg_certificate_cer_file_name;
extern wchar_t *cfg_certificate_key_file_name;

extern char *g_certificate_pkcs_file_name;
extern char *g_certificate_pkcs_password;

extern char *g_certificate_cer_file_name;
extern char *g_certificate_key_file_name;

extern bool cfg_use_authentication;
extern wchar_t *cfg_authentication_username;
extern wchar_t *cfg_authentication_password;
extern unsigned char cfg_authentication_type;

// HTTP proxy
extern bool cfg_enable_proxy;
extern unsigned char cfg_address_type;	// 0 = Host name, 1 = IP address
extern unsigned long cfg_ip_address;
extern wchar_t *cfg_hostname;
extern unsigned short cfg_port;

extern wchar_t *cfg_proxy_auth_username;
extern wchar_t *cfg_proxy_auth_password;

extern wchar_t *g_punycode_hostname;

extern char *g_proxy_auth_username;
extern char *g_proxy_auth_password;
extern char *g_proxy_auth_key;
extern unsigned long g_proxy_auth_key_length;

// HTTPS proxy
extern bool cfg_enable_proxy_s;
extern unsigned char cfg_address_type_s;	// 0 = Host name, 1 = IP address
extern unsigned long cfg_ip_address_s;
extern wchar_t *cfg_hostname_s;
extern unsigned short cfg_port_s;

extern wchar_t *cfg_proxy_auth_username_s;
extern wchar_t *cfg_proxy_auth_password_s;

extern wchar_t *g_punycode_hostname_s;

extern char *g_proxy_auth_username_s;
extern char *g_proxy_auth_password_s;
extern char *g_proxy_auth_key_s;
extern unsigned long g_proxy_auth_key_length_s;

// SOCKS proxy
extern bool cfg_enable_proxy_socks;
extern unsigned char cfg_socks_type;			// 0 = SOCKS 4, 1 = SOCKS 5
extern unsigned char cfg_address_type_socks;	// 0 = Host name, 1 = IP address
extern unsigned long cfg_ip_address_socks;
extern wchar_t *cfg_hostname_socks;
extern unsigned short cfg_port_socks;
extern bool cfg_use_authentication_socks;
extern bool cfg_resolve_domain_names_v4a;		// Proxy server resolves the domain names.
extern bool cfg_resolve_domain_names;			// Proxy server resolves the domain names.

extern wchar_t *cfg_proxy_auth_username_socks;
extern wchar_t *cfg_proxy_auth_password_socks;

extern wchar_t *cfg_proxy_auth_ident_username_socks;

extern wchar_t *g_punycode_hostname_socks;

extern char *g_proxy_auth_username_socks;
extern char *g_proxy_auth_password_socks;

extern char *g_proxy_auth_ident_username_socks;

//

extern unsigned short cfg_timeout;

//

extern unsigned long long cfg_download_speed_limit;

//

extern bool cfg_resume_downloads;

extern unsigned long long cfg_max_file_size;
extern unsigned char cfg_prompt_last_modified;	// 0 = Display Prompt, 1 = Continue Download, 2 = Restart Download, 3 = Skip Download
extern unsigned char cfg_prompt_rename;			// 0 = Display Prompt, 1 = Rename File, 2 = Overwrite File, 3 = Skip Download
extern unsigned char cfg_prompt_file_size;		// 0 = Display Prompt, 1 = Continue Download, 2 = Skip Download

extern bool cfg_use_temp_download_directory;
extern wchar_t *cfg_temp_download_directory;

//

extern bool cfg_show_tray_progress;
extern unsigned char cfg_drop_window_transparency;
extern bool cfg_show_drop_window_progress;

//

// Appearance

extern int cfg_sorted_column_index;
extern unsigned char cfg_sorted_direction;
extern bool cfg_sort_added_and_updating_items;
extern bool cfg_expand_added_group_items;
extern bool cfg_scroll_to_last_item;
extern bool cfg_show_embedded_icon;

extern bool cfg_show_gridlines;
extern bool cfg_show_part_progress;

extern bool cfg_draw_full_rows;
extern bool cfg_draw_all_rows;

extern FONT_SETTINGS cfg_even_row_font_settings;
extern FONT_SETTINGS cfg_odd_row_font_settings;

extern COLORREF cfg_background_color;
extern COLORREF cfg_gridline_color;
extern COLORREF cfg_selection_marquee_color;

extern COLORREF cfg_even_row_background_color;
extern COLORREF cfg_odd_row_background_color;

extern COLORREF cfg_even_row_highlight_color;
extern COLORREF cfg_odd_row_highlight_color;

extern COLORREF cfg_even_row_highlight_font_color;
extern COLORREF cfg_odd_row_highlight_font_color;

extern COLORREF cfg_color_t_d_p;	// Tray downloading progress
extern COLORREF cfg_color_t_d_b;	// Tray downloading border
extern COLORREF cfg_color_t_p_p;	// Tray paused progress
extern COLORREF cfg_color_t_p_b;	// Tray paused border
extern COLORREF cfg_color_t_e_p;	// Tray error progress
extern COLORREF cfg_color_t_e_b;	// Tray error border

extern COLORREF cfg_color_d_d_p;	// Drop downloading progress
extern COLORREF cfg_color_d_d_b;	// Drop downloading border
extern COLORREF cfg_color_d_p_p;	// Drop paused progress
extern COLORREF cfg_color_d_p_b;	// Drop paused border
extern COLORREF cfg_color_d_e_p;	// Drop error progress
extern COLORREF cfg_color_d_e_b;	// Drop error border

extern COLORREF cfg_color_1a, cfg_color_1b, cfg_color_1c, cfg_color_1d, cfg_color_1e;
extern COLORREF cfg_color_2a, cfg_color_2b, cfg_color_2c, cfg_color_2d, cfg_color_2e;
extern COLORREF cfg_color_3a, cfg_color_3b, cfg_color_3c, cfg_color_3d, cfg_color_3e;
extern COLORREF cfg_color_4a, cfg_color_4b, cfg_color_4c, cfg_color_4d, cfg_color_4e;
extern COLORREF cfg_color_5a, cfg_color_5b, cfg_color_5c, cfg_color_5d, cfg_color_5e;
extern COLORREF cfg_color_6a, cfg_color_6b, cfg_color_6c, cfg_color_6d, cfg_color_6e;
extern COLORREF cfg_color_7a, cfg_color_7b, cfg_color_7c, cfg_color_7d, cfg_color_7e;
extern COLORREF cfg_color_8a, cfg_color_8b, cfg_color_8c, cfg_color_8d, cfg_color_8e;
extern COLORREF cfg_color_9a, cfg_color_9b, cfg_color_9c, cfg_color_9d, cfg_color_9e;
extern COLORREF cfg_color_10a, cfg_color_10b, cfg_color_10c, cfg_color_10d, cfg_color_10e;
extern COLORREF cfg_color_11a, cfg_color_11b, cfg_color_11c, cfg_color_11d, cfg_color_11e;
extern COLORREF cfg_color_12a, cfg_color_12b, cfg_color_12c, cfg_color_12d, cfg_color_12e;
extern COLORREF cfg_color_13a, cfg_color_13b, cfg_color_13c, cfg_color_13d, cfg_color_13e;
extern COLORREF cfg_color_14a, cfg_color_14b, cfg_color_14c, cfg_color_14d, cfg_color_14e;
extern COLORREF cfg_color_15a, cfg_color_15b, cfg_color_15c, cfg_color_15d, cfg_color_15e;
extern COLORREF cfg_color_16a, cfg_color_16b, cfg_color_16c, cfg_color_16d, cfg_color_16e;

//

extern COLORREF *progress_colors[ NUM_COLORS ];
extern COLORREF *td_progress_colors[ TD_NUM_COLORS ];

extern COLORREF g_CustColors[ 16 ];

extern char *download_columns[ NUM_COLUMNS ];
extern int *download_columns_width[ NUM_COLUMNS ];

extern unsigned char g_total_columns;

extern HANDLE g_timeout_semaphore;	// For updating the connection states.
extern HANDLE g_timer_semaphore;	// For updating the listview.

#define NUM_SESSION_STATUS	9

extern unsigned int g_session_status_count[ NUM_SESSION_STATUS ];	// 9 states that can be considered finished (Completed, Stopped, Failed, etc.)

extern bool g_timers_running;

extern SYSTEMTIME g_compile_time;

extern bool g_is_windows_8_or_higher;
extern bool g_can_use_tls_1_3;

extern bool g_use_openssl;

extern bool g_can_perform_shutdown_action;
extern bool g_perform_shutdown_action;

extern bool edit_from_menu;			// True if we activate the edit from our (rename) menu, or Ctrl + R.

#endif
