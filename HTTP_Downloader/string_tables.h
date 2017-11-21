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

#ifndef _STRING_TABLES_H
#define _STRING_TABLES_H

// Values with ":", "&", or "-" are replaced with _
// Values with "..." are replaced with ___ 
// Values with # are replaced with NUM
// Keep the defines case sensitive.
// Try to keep them in order.

extern wchar_t *month_string_table[];
extern wchar_t *day_string_table[];

extern wchar_t *download_string_table[];

extern wchar_t *menu_string_table[];

#define ST_NUM						menu_string_table[ 0 ]
#define ST__About					menu_string_table[ 1 ]
#define ST__Add_URL_s____			menu_string_table[ 2 ]
#define ST__Edit					menu_string_table[ 3 ]
#define ST__File					menu_string_table[ 4 ]
#define ST__Help					menu_string_table[ 5 ]
#define ST__Options___				menu_string_table[ 6 ]
#define ST__Pause					menu_string_table[ 7 ]
#define ST__Remove_Selected			menu_string_table[ 8 ]
#define ST__Select_All				menu_string_table[ 9 ]
#define ST__Tools					menu_string_table[ 10 ]
#define ST__View					menu_string_table[ 11 ]
#define ST_Active_Parts				menu_string_table[ 12 ]
#define ST_Add_URL_s____			menu_string_table[ 13 ]
#define ST_Date_and_Time_Added		menu_string_table[ 14 ]
#define ST_Download_Directory		menu_string_table[ 15 ]
#define ST_Download_Speed			menu_string_table[ 16 ]
#define ST_Downloaded				menu_string_table[ 17 ]
#define ST_E_xit					menu_string_table[ 18 ]
#define ST_Exit						menu_string_table[ 19 ]
#define ST_File_Size				menu_string_table[ 20 ]
#define ST_File_Type				menu_string_table[ 21 ]
#define ST_Filename					menu_string_table[ 22 ]
#define ST_Move_Down				menu_string_table[ 23 ]
#define ST_Move_To_Bottom			menu_string_table[ 24 ]
#define ST_Move_To_Top				menu_string_table[ 25 ]
#define ST_Move_Up					menu_string_table[ 26 ]
#define ST_Open_Directory			menu_string_table[ 27 ]
#define ST_Open_File				menu_string_table[ 28 ]
#define ST_Open_Download_List		menu_string_table[ 29 ]
#define ST_Options___				menu_string_table[ 30 ]
#define ST_Pause					menu_string_table[ 31 ]
#define ST_Pause_Active				menu_string_table[ 32 ]
#define ST_Progress					menu_string_table[ 33 ]
#define ST_Queue					menu_string_table[ 34 ]
#define ST_Remove_Completed			menu_string_table[ 35 ]
#define ST_Remove_Selected			menu_string_table[ 36 ]
#define ST_Select_All				menu_string_table[ 37 ]
#define ST_St_art					menu_string_table[ 38 ]
#define ST_Start					menu_string_table[ 39 ]
#define ST__Status_Bar				menu_string_table[ 40 ]
#define ST_St_op					menu_string_table[ 41 ]
#define ST_Stop						menu_string_table[ 42 ]
#define ST_Stop_All					menu_string_table[ 43 ]
#define ST_Time_Elapsed				menu_string_table[ 44 ]
#define ST_Time_Remaining			menu_string_table[ 45 ]
#define ST_TLS___SSL_Version		menu_string_table[ 46 ]
#define ST_URL						menu_string_table[ 47 ]

extern wchar_t *options_string_table[];

#define ST__									options_string_table[ 0 ]
#define ST_Active_download_limit_				options_string_table[ 1 ]
#define ST_Always_on_top						options_string_table[ 2 ]
#define ST_Apply								options_string_table[ 3 ]
#define ST_Basic_Authentication					options_string_table[ 4 ]
#define ST_Certificate_file_					options_string_table[ 5 ]
#define ST_Close_to_System_Tray					options_string_table[ 6 ]
#define ST_Configure_Proxies					options_string_table[ 7 ]
#define ST_Connection							options_string_table[ 8 ]
#define ST_Default_download_directory_			options_string_table[ 9 ]
#define ST_Default_download_parts_				options_string_table[ 10 ]
#define ST_Default_SSL___TLS_version_			options_string_table[ 11 ]
#define ST_Digest_Authentication				options_string_table[ 12 ]
#define ST_Enable_download_history				options_string_table[ 13 ]
#define ST_Enable_HTTP_proxy_					options_string_table[ 14 ]
#define ST_Enable_HTTPS_proxy_					options_string_table[ 15 ]
#define ST_Enable_quick_file_allocation			options_string_table[ 16 ]
#define ST_Enable_server_						options_string_table[ 17 ]
#define ST_Enable_SSL___TLS_					options_string_table[ 18 ]
#define ST_Enable_System_Tray_icon_				options_string_table[ 19 ]
#define ST_General								options_string_table[ 20 ]
#define ST_Hostname___IPv6_address_				options_string_table[ 21 ]
#define ST_IPv4_address_						options_string_table[ 22 ]
#define ST_Key_file_							options_string_table[ 23 ]
#define ST_Load_PKCS_NUM12_File					options_string_table[ 24 ]
#define ST_Load_Private_Key_File				options_string_table[ 25 ]
#define ST_Load_X_509_Certificate_File			options_string_table[ 26 ]
#define ST_Minimize_to_System_Tray				options_string_table[ 27 ]
#define ST_OK									options_string_table[ 28 ]
#define ST_PKCS_NUM12_							options_string_table[ 29 ]
#define ST_PKCS_NUM12_file_						options_string_table[ 30 ]
#define ST_PKCS_NUM12_password_					options_string_table[ 31 ]
#define ST_Port_								options_string_table[ 32 ]
#define ST_Proxy								options_string_table[ 33 ]
#define ST_Public___Private_key_pair_			options_string_table[ 34 ]
#define ST_Require_authentication_				options_string_table[ 35 ]
#define ST_Retry_incomplete_downloads_			options_string_table[ 36 ]
#define ST_Retry_incomplete_parts_				options_string_table[ 37 ]
#define ST_Server_SSL___TLS_version_			options_string_table[ 38 ]
#define ST_Thread_pool_count_					options_string_table[ 39 ]
#define ST_Timeout__seconds__					options_string_table[ 40 ]

extern wchar_t *add_urls_string_table[];

#define ST_Advanced_AB							add_urls_string_table[ 0 ]
#define ST_Advanced_BB							add_urls_string_table[ 1 ]
#define	ST_Authentication						add_urls_string_table[ 2 ]
#define ST_Cookies_								add_urls_string_table[ 3 ]
#define ST_Download								add_urls_string_table[ 4 ]
#define ST_Download_directory_					add_urls_string_table[ 5 ]
#define ST_Download_parts_						add_urls_string_table[ 6 ]
#define ST_Headers_								add_urls_string_table[ 7 ]
#define ST_Password_							add_urls_string_table[ 8 ]
#define ST_Simulate_download					add_urls_string_table[ 9 ]
#define ST_SSL___TLS_version_					add_urls_string_table[ 10 ]
#define ST_URL_s__								add_urls_string_table[ 11 ]
#define ST_Username_							add_urls_string_table[ 12 ]

extern wchar_t *common_string_table[];

#define ST_BTN___								common_string_table[ 0 ]
#define ST__Simulated_							common_string_table[ 1 ]
#define ST_Add_URL_s_							common_string_table[ 2 ]
#define ST_Allocating_File						common_string_table[ 3 ]
#define ST_Authorization_Required				common_string_table[ 4 ]
#define ST_Cancel								common_string_table[ 5 ]
#define ST_Completed							common_string_table[ 6 ]
#define ST_Connecting							common_string_table[ 7 ]
#define ST_Download_speed_						common_string_table[ 8 ]
#define ST_Download_speed__0_B_s				common_string_table[ 9 ]
#define ST_Download_speed__0_00_GB_s			common_string_table[ 10 ]
#define ST_Download_speed__0_00_KB_s			common_string_table[ 11 ]
#define ST_Download_speed__0_00_MB_s			common_string_table[ 12 ]
#define ST_Failed								common_string_table[ 13 ]
#define ST_File_IO_Error						common_string_table[ 14 ]
#define ST_Options								common_string_table[ 15 ]
#define ST_Paused								common_string_table[ 16 ]
#define ST_Proxy_Authentication_Required		common_string_table[ 17 ]
#define ST_Queued								common_string_table[ 18 ]
#define ST_Skipped								common_string_table[ 19 ]
#define ST_Stopped								common_string_table[ 20 ]
#define ST_SSL_2_0								common_string_table[ 21 ]
#define ST_SSL_3_0								common_string_table[ 22 ]
#define ST_Timed_Out							common_string_table[ 23 ]
#define ST_TLS_1_0								common_string_table[ 24 ]
#define ST_TLS_1_1								common_string_table[ 25 ]
#define ST_TLS_1_2								common_string_table[ 26 ]
#define ST_Total_downloaded_					common_string_table[ 27 ]
#define ST_Total_downloaded__0_B				common_string_table[ 28 ]
#define ST_Total_downloaded__0_00_GB			common_string_table[ 29 ]
#define ST_Total_downloaded__0_00_KB			common_string_table[ 30 ]
#define ST_Total_downloaded__0_00_MB			common_string_table[ 31 ]

extern wchar_t *common_message_string_table[];

#define ST_A_restart_is_required					common_message_string_table[ 0 ]
#define ST_A_restart_is_required_allocation			common_message_string_table[ 1 ]
#define ST_A_restart_is_required_threads			common_message_string_table[ 2 ]
#define ST_Select_the_default_download_directory	common_message_string_table[ 3 ]
#define ST_Select_the_download_directory			common_message_string_table[ 4 ]
#define ST_The_specified_file_was_not_found			common_message_string_table[ 5 ]
#define ST_The_specified_path_was_not_found			common_message_string_table[ 6 ]
#define ST_You_must_supply_download_directory		common_message_string_table[ 7 ]

#endif
