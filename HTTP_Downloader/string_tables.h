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

#ifndef _STRING_TABLES_H
#define _STRING_TABLES_H

// Values with ":", "&", or "-" are replaced with _
// Values with "..." are replaced with ___ 
// Values with # are replaced with NUM
// Keep the defines case sensitive.
// Try to keep them in order.

#define MONTH_STRING_TABLE_SIZE					12
#define DAY_STRING_TABLE_SIZE					7
#define DOWNLOAD_STRING_TABLE_SIZE				17
#define MENU_STRING_TABLE_SIZE					81

#define OPTIONS_STRING_TABLE_SIZE				11
#define OPTIONS_ADVANCED_STRING_TABLE_SIZE		38
#define OPTIONS_APPEARANCE_STRING_TABLE_SIZE	34
#define OPTIONS_CONNECTION_STRING_TABLE_SIZE	9
#define OPTIONS_FTP_STRING_TABLE_SIZE			9
#define OPTIONS_GENERAL_STRING_TABLE_SIZE		15
#define OPTIONS_PROXY_STRING_TABLE_SIZE			9
#define OPTIONS_SERVER_STRING_TABLE_SIZE		21
#define OPTIONS_SFTP_STRING_TABLE_SIZE			22

#define CMESSAGEBOX_STRING_TABLE_SIZE			7

#define ADD_URLS_STRING_TABLE_SIZE				23
#define SEARCH_STRING_TABLE_SIZE				8
#define SITE_MANAGER_STRING_TABLE_SIZE			24
#define ADD_CATEGORY_STRING_TABLE_SIZE			1
#define FINGERPRINT_PROMPT_STRING_TABLE_SIZE	2
#define UPDATE_CHECK_STRING_TABLE_SIZE			9
#define COMMON_STRING_TABLE_SIZE				68
#define COMMON_MESSAGE_STRING_TABLE_SIZE		39

#define ABOUT_STRING_TABLE_SIZE					5
#define DYNAMIC_MESSAGE_STRING_TABLE_SIZE		4

#define SFTP_KEX_STRING_TABLE_SIZE				5
#define SFTP_HK_STRING_TABLE_SIZE				4
#define SFTP_EC_STRING_TABLE_SIZE				6
#define SFTP_HKA_STRING_TABLE_SIZE				6

#define FILE_FILTERS_STRING_TABLE_SIZE			8

#define TOTAL_LOCALE_STRINGS	( MONTH_STRING_TABLE_SIZE + \
								  DAY_STRING_TABLE_SIZE + \
								  DOWNLOAD_STRING_TABLE_SIZE + \
								  MENU_STRING_TABLE_SIZE + \
								  OPTIONS_STRING_TABLE_SIZE + \
								  OPTIONS_ADVANCED_STRING_TABLE_SIZE + \
								  OPTIONS_APPEARANCE_STRING_TABLE_SIZE + \
								  OPTIONS_CONNECTION_STRING_TABLE_SIZE + \
								  OPTIONS_FTP_STRING_TABLE_SIZE + \
								  OPTIONS_GENERAL_STRING_TABLE_SIZE + \
								  OPTIONS_PROXY_STRING_TABLE_SIZE + \
								  OPTIONS_SERVER_STRING_TABLE_SIZE + \
								  OPTIONS_SFTP_STRING_TABLE_SIZE + \
								  CMESSAGEBOX_STRING_TABLE_SIZE + \
								  ADD_URLS_STRING_TABLE_SIZE + \
								  SEARCH_STRING_TABLE_SIZE + \
								  SITE_MANAGER_STRING_TABLE_SIZE + \
								  ADD_CATEGORY_STRING_TABLE_SIZE + \
								  FINGERPRINT_PROMPT_STRING_TABLE_SIZE + \
								  UPDATE_CHECK_STRING_TABLE_SIZE + \
								  COMMON_STRING_TABLE_SIZE + \
								  COMMON_MESSAGE_STRING_TABLE_SIZE + \
								  ABOUT_STRING_TABLE_SIZE + \
								  DYNAMIC_MESSAGE_STRING_TABLE_SIZE + \
								  SFTP_KEX_STRING_TABLE_SIZE + \
								  SFTP_HK_STRING_TABLE_SIZE + \
								  SFTP_EC_STRING_TABLE_SIZE + \
								  SFTP_HKA_STRING_TABLE_SIZE + \
								  FILE_FILTERS_STRING_TABLE_SIZE )

#define MONTH_STRING_TABLE_OFFSET				0
#define DAY_STRING_TABLE_OFFSET					( MONTH_STRING_TABLE_OFFSET + MONTH_STRING_TABLE_SIZE )
#define DOWNLOAD_STRING_TABLE_OFFSET			( DAY_STRING_TABLE_OFFSET + DAY_STRING_TABLE_SIZE )
#define MENU_STRING_TABLE_OFFSET				( DOWNLOAD_STRING_TABLE_OFFSET + DOWNLOAD_STRING_TABLE_SIZE )
#define OPTIONS_STRING_TABLE_OFFSET				( MENU_STRING_TABLE_OFFSET + MENU_STRING_TABLE_SIZE )
#define OPTIONS_ADVANCED_STRING_TABLE_OFFSET	( OPTIONS_STRING_TABLE_OFFSET + OPTIONS_STRING_TABLE_SIZE )
#define OPTIONS_APPEARANCE_STRING_TABLE_OFFSET	( OPTIONS_ADVANCED_STRING_TABLE_OFFSET + OPTIONS_ADVANCED_STRING_TABLE_SIZE )
#define OPTIONS_CONNECTION_STRING_TABLE_OFFSET	( OPTIONS_APPEARANCE_STRING_TABLE_OFFSET + OPTIONS_APPEARANCE_STRING_TABLE_SIZE )
#define OPTIONS_FTP_STRING_TABLE_OFFSET			( OPTIONS_CONNECTION_STRING_TABLE_OFFSET + OPTIONS_CONNECTION_STRING_TABLE_SIZE )
#define OPTIONS_GENERAL_STRING_TABLE_OFFSET		( OPTIONS_FTP_STRING_TABLE_OFFSET + OPTIONS_FTP_STRING_TABLE_SIZE )
#define OPTIONS_PROXY_STRING_TABLE_OFFSET		( OPTIONS_GENERAL_STRING_TABLE_OFFSET + OPTIONS_GENERAL_STRING_TABLE_SIZE )
#define OPTIONS_SERVER_STRING_TABLE_OFFSET		( OPTIONS_PROXY_STRING_TABLE_OFFSET + OPTIONS_PROXY_STRING_TABLE_SIZE )
#define OPTIONS_SFTP_STRING_TABLE_OFFSET		( OPTIONS_SERVER_STRING_TABLE_OFFSET + OPTIONS_SERVER_STRING_TABLE_SIZE )
#define CMESSAGEBOX_STRING_TABLE_OFFSET			( OPTIONS_SFTP_STRING_TABLE_OFFSET + OPTIONS_SFTP_STRING_TABLE_SIZE )
#define ADD_URLS_STRING_TABLE_OFFSET			( CMESSAGEBOX_STRING_TABLE_OFFSET + CMESSAGEBOX_STRING_TABLE_SIZE )
#define SEARCH_STRING_TABLE_OFFSET				( ADD_URLS_STRING_TABLE_OFFSET + ADD_URLS_STRING_TABLE_SIZE )
#define SITE_MANAGER_STRING_TABLE_OFFSET		( SEARCH_STRING_TABLE_OFFSET + SEARCH_STRING_TABLE_SIZE )
#define ADD_CATEGORY_STRING_TABLE_OFFSET		( SITE_MANAGER_STRING_TABLE_OFFSET + SITE_MANAGER_STRING_TABLE_SIZE )
#define FINGERPRINT_PROMPT_STRING_TABLE_OFFSET	( ADD_CATEGORY_STRING_TABLE_OFFSET + ADD_CATEGORY_STRING_TABLE_SIZE )
#define UPDATE_CHECK_TABLE_OFFSET				( FINGERPRINT_PROMPT_STRING_TABLE_OFFSET + FINGERPRINT_PROMPT_STRING_TABLE_SIZE )
#define COMMON_STRING_TABLE_OFFSET				( UPDATE_CHECK_TABLE_OFFSET + UPDATE_CHECK_STRING_TABLE_SIZE )
#define COMMON_MESSAGE_STRING_TABLE_OFFSET		( COMMON_STRING_TABLE_OFFSET + COMMON_STRING_TABLE_SIZE )
#define ABOUT_STRING_TABLE_OFFSET				( COMMON_MESSAGE_STRING_TABLE_OFFSET + COMMON_MESSAGE_STRING_TABLE_SIZE )
#define DYNAMIC_MESSAGE_STRING_TABLE_OFFSET		( ABOUT_STRING_TABLE_OFFSET + ABOUT_STRING_TABLE_SIZE )
#define SFTP_KEX_STRING_TABLE_OFFSET			( DYNAMIC_MESSAGE_STRING_TABLE_OFFSET + DYNAMIC_MESSAGE_STRING_TABLE_SIZE )
#define SFTP_HK_STRING_TABLE_OFFSET				( SFTP_KEX_STRING_TABLE_OFFSET + SFTP_KEX_STRING_TABLE_SIZE )
#define SFTP_EC_STRING_TABLE_OFFSET				( SFTP_HK_STRING_TABLE_OFFSET + SFTP_HK_STRING_TABLE_SIZE )
#define SFTP_HKA_STRING_TABLE_OFFSET			( SFTP_EC_STRING_TABLE_OFFSET + SFTP_HKA_STRING_TABLE_SIZE )
#define FILE_FILTERS_STRING_TABLE_OFFSET		( SFTP_HKA_STRING_TABLE_OFFSET + FILE_FILTERS_STRING_TABLE_SIZE )

struct STRING_TABLE_DATA
{
	wchar_t *value;
	unsigned short length;
};

extern bool g_use_dynamic_locale;	// Did we get the strings from a file? We'll need to free the memory when we're done.

extern STRING_TABLE_DATA g_locale_table[ TOTAL_LOCALE_STRINGS ];

extern STRING_TABLE_DATA month_string_table[];
extern STRING_TABLE_DATA day_string_table[];
extern STRING_TABLE_DATA download_string_table[];
extern STRING_TABLE_DATA menu_string_table[];
extern STRING_TABLE_DATA options_string_table[];
extern STRING_TABLE_DATA options_advanced_string_table[];
extern STRING_TABLE_DATA options_appearance_string_table[];
extern STRING_TABLE_DATA options_connection_string_table[];
extern STRING_TABLE_DATA options_ftp_string_table[];
extern STRING_TABLE_DATA options_general_string_table[];
extern STRING_TABLE_DATA options_proxy_string_table[];
extern STRING_TABLE_DATA options_server_string_table[];
extern STRING_TABLE_DATA options_sftp_string_table[];
extern STRING_TABLE_DATA cmessagebox_string_table[];
extern STRING_TABLE_DATA add_urls_string_table[];
extern STRING_TABLE_DATA search_string_table[];
extern STRING_TABLE_DATA site_manager_string_table[];
extern STRING_TABLE_DATA add_category_string_table[];
extern STRING_TABLE_DATA fingerprint_prompt_string_table[];
extern STRING_TABLE_DATA update_check_string_table[];
extern STRING_TABLE_DATA common_string_table[];
extern STRING_TABLE_DATA common_message_string_table[];
extern STRING_TABLE_DATA about_string_table[];
extern STRING_TABLE_DATA dynamic_message_string_table[];
extern STRING_TABLE_DATA sftp_kex_string_table[];
extern STRING_TABLE_DATA sftp_hk_string_table[];
extern STRING_TABLE_DATA sftp_ec_string_table[];
extern STRING_TABLE_DATA sftp_hka_string_table[];
extern STRING_TABLE_DATA file_filters_string_table[];

void InitializeLocaleValues();
void UninitializeLocaleValues();

// Month
#define ST_V_January									g_locale_table[ 0 ].value
#define ST_V_February									g_locale_table[ 1 ].value
#define ST_V_March										g_locale_table[ 2 ].value
#define ST_V_April										g_locale_table[ 3 ].value
#define ST_V_May										g_locale_table[ 4 ].value
#define ST_V_June										g_locale_table[ 5 ].value
#define ST_V_July										g_locale_table[ 6 ].value
#define ST_V_August										g_locale_table[ 7 ].value
#define ST_V_September									g_locale_table[ 8 ].value
#define ST_V_October									g_locale_table[ 9 ].value
#define ST_V_November									g_locale_table[ 10 ].value
#define ST_V_December									g_locale_table[ 11 ].value

// Day
#define ST_V_Sunday										g_locale_table[ 12 ].value
#define ST_V_Monday										g_locale_table[ 13 ].value
#define ST_V_Tuesday									g_locale_table[ 14 ].value
#define ST_V_Wednesday									g_locale_table[ 15 ].value
#define ST_V_Thursday									g_locale_table[ 16 ].value
#define ST_V_Friday										g_locale_table[ 17 ].value
#define ST_V_Saturday									g_locale_table[ 18 ].value
/*
// Download
#define ST_V_NUM										g_locale_table[ 19 ].value
#define ST_V_Active_Parts								g_locale_table[ 20 ].value
#define ST_V_Category									g_locale_table[ 21 ].value
#define ST_V_Comments									g_locale_table[ 22 ].value
#define ST_V_Date_and_Time_Added						g_locale_table[ 23 ].value
#define ST_V_Download_Directory							g_locale_table[ 24 ].value
#define ST_V_Download_Speed								g_locale_table[ 25 ].value
#define ST_V_Download_Speed_Limit						g_locale_table[ 26 ].value
#define ST_V_Downloaded									g_locale_table[ 27 ].value
#define ST_V_File_Size									g_locale_table[ 28 ].value
#define ST_V_File_Type									g_locale_table[ 29 ].value
#define ST_V_Filename									g_locale_table[ 30 ].value
#define ST_V_Progress									g_locale_table[ 31 ].value
#define ST_V_SSL___TLS_Version							g_locale_table[ 32 ].value
#define ST_V_Time_Elapsed								g_locale_table[ 33 ].value
#define ST_V_Time_Remaining								g_locale_table[ 34 ].value
#define ST_V_URL										g_locale_table[ 35 ].value
*/
// Menu
#define ST_V_NUM										g_locale_table[ 36 ].value
#define ST_V__About										g_locale_table[ 37 ].value
#define ST_V_Active_Parts								g_locale_table[ 38 ].value
#define ST_V_Add_Category___							g_locale_table[ 39 ].value
#define ST_V__Add_URL_s____								g_locale_table[ 40 ].value
#define ST_V_Add_URL_s____								g_locale_table[ 41 ].value
#define ST_V_Always_on_Top								g_locale_table[ 42 ].value
#define ST_V_C_ategories								g_locale_table[ 43 ].value
#define ST_V_Category									g_locale_table[ 44 ].value
#define ST_V__Check_for_Updates___						g_locale_table[ 45 ].value
#define ST_V__Column_Headers							g_locale_table[ 46 ].value
#define ST_V_Comments									g_locale_table[ 47 ].value
#define ST_V__Copy_URL_s_								g_locale_table[ 48 ].value
#define ST_V_Copy_URL_s_								g_locale_table[ 49 ].value
#define ST_V_Date_and_Time_Added						g_locale_table[ 50 ].value
#define ST_V__Delete_									g_locale_table[ 51 ].value
#define ST_V_Delete										g_locale_table[ 52 ].value
#define ST_V_Download_Directory							g_locale_table[ 53 ].value
#define ST_V_Download_Speed								g_locale_table[ 54 ].value
#define ST_V_Download_Speed_Limit						g_locale_table[ 55 ].value
#define ST_V_Downloaded									g_locale_table[ 56 ].value
#define ST_V__Edit										g_locale_table[ 57 ].value
#define ST_V_Enable_List__Edit_Mode						g_locale_table[ 58 ].value
#define ST_V_Enable_List_Edit_Mode						g_locale_table[ 59 ].value
#define ST_V_E_xit										g_locale_table[ 60 ].value
#define ST_V_Exit										g_locale_table[ 61 ].value
#define ST_V__Export_Download_History___				g_locale_table[ 62 ].value
#define ST_V__File										g_locale_table[ 63 ].value
#define ST_V_File_Size									g_locale_table[ 64 ].value
#define ST_V_File_Type									g_locale_table[ 65 ].value
#define ST_V_Filename									g_locale_table[ 66 ].value
#define ST_V_Global_Download_Speed__Limit____			g_locale_table[ 67 ].value
#define ST_V__Help										g_locale_table[ 68 ].value
#define ST_V_HTTP_Downloader__Home_Page					g_locale_table[ 69 ].value
#define ST_V__Import_Download_History___				g_locale_table[ 70 ].value
#define ST_V_Move_Down									g_locale_table[ 71 ].value
#define ST_V_Move_to_Bottom								g_locale_table[ 72 ].value
#define ST_V_Move_to_Top								g_locale_table[ 73 ].value
#define ST_V_Move_Up									g_locale_table[ 74 ].value
#define ST_V_Open_Directory								g_locale_table[ 75 ].value
#define ST_V_Open_File									g_locale_table[ 76 ].value
#define ST_V_Open_Download_List							g_locale_table[ 77 ].value
#define ST_V__Options____								g_locale_table[ 78 ].value
#define ST_V_Options___									g_locale_table[ 79 ].value
#define ST_V__Pause										g_locale_table[ 80 ].value
#define ST_V_Pause										g_locale_table[ 81 ].value
#define ST_V_Pause_Active								g_locale_table[ 82 ].value
#define ST_V_Progress									g_locale_table[ 83 ].value
#define ST_V_Queue										g_locale_table[ 84 ].value
#define ST_V__Remove_									g_locale_table[ 85 ].value
#define ST_V_Remove										g_locale_table[ 86 ].value
#define ST_V_Remove_and_Delete_							g_locale_table[ 87 ].value
#define ST_V_Remove_and_Delete							g_locale_table[ 88 ].value
#define ST_V_Remove_Completed_							g_locale_table[ 89 ].value
#define ST_V_Rename_									g_locale_table[ 90 ].value
#define ST_V_Rename										g_locale_table[ 91 ].value
#define ST_V_Restart									g_locale_table[ 92 ].value
#define ST_V_Resu_me									g_locale_table[ 93 ].value
#define ST_V_Resume										g_locale_table[ 94 ].value
#define ST_V__Save_Download_History___					g_locale_table[ 95 ].value
#define ST_V__Search____								g_locale_table[ 96 ].value
#define ST_V__Select_All_								g_locale_table[ 97 ].value
#define ST_V_Select_All									g_locale_table[ 98 ].value
#define ST_V_Site__Manager____							g_locale_table[ 99 ].value
#define ST_V_SSL___TLS_Version							g_locale_table[ 100 ].value
#define ST_V_St_art										g_locale_table[ 101 ].value
#define ST_V_Start										g_locale_table[ 102 ].value
#define ST_V_Start___Resume_Inactive					g_locale_table[ 103 ].value
#define ST_V__Status_Bar								g_locale_table[ 104 ].value
#define ST_V_St_op										g_locale_table[ 105 ].value
#define ST_V_Stop										g_locale_table[ 106 ].value
#define ST_V_Stop_All									g_locale_table[ 107 ].value
#define ST_V_Time_Elapsed								g_locale_table[ 108 ].value
#define ST_V_Time_Remaining								g_locale_table[ 109 ].value
#define ST_V__Toolbar									g_locale_table[ 110 ].value
#define ST_V__Tools										g_locale_table[ 111 ].value
#define ST_V_Update_Category___							g_locale_table[ 112 ].value
#define ST_V__Update_Download____						g_locale_table[ 113 ].value
#define ST_V_Update_Download___							g_locale_table[ 114 ].value
#define ST_V_URL										g_locale_table[ 115 ].value
#define ST_V__View										g_locale_table[ 116 ].value

// Options
#define ST_V_Advanced									g_locale_table[ 117 ].value
#define ST_V_Appearance									g_locale_table[ 118 ].value
#define ST_V_Apply										g_locale_table[ 119 ].value
#define ST_V_Connection									g_locale_table[ 120 ].value
#define ST_V_Fingerprints								g_locale_table[ 121 ].value
#define ST_V_FTP										g_locale_table[ 122 ].value
#define ST_V_General									g_locale_table[ 123 ].value
#define ST_V_OK											g_locale_table[ 124 ].value
#define ST_V_Private_Keys								g_locale_table[ 125 ].value
#define ST_V_Proxy										g_locale_table[ 126 ].value
#define ST_V_SFTP										g_locale_table[ 127 ].value

// Options Advanced
#define ST_V_Add_in_Stopped_state						g_locale_table[ 128 ].value
#define ST_V_Allow_only_one_instance					g_locale_table[ 129 ].value
#define ST_V_Apply_initially_set_proxy					g_locale_table[ 130 ].value
#define ST_V_Continue_Download							g_locale_table[ 131 ].value
#define ST_V_Default_download_directory_				g_locale_table[ 132 ].value
#define ST_V_Display_Prompt								g_locale_table[ 133 ].value
#define ST_V_Download_immediately						g_locale_table[ 134 ].value
#define ST_V_Download_non_200_and_non_206_responses		g_locale_table[ 135 ].value
#define ST_V_Drag_and_drop_URL_s__action_				g_locale_table[ 136 ].value
#define ST_V_Enable_download_history					g_locale_table[ 137 ].value
#define ST_V_Enable_quick_file_allocation				g_locale_table[ 138 ].value
#define ST_V_Enable_sparse_file_allocation				g_locale_table[ 139 ].value
#define ST_V_Exit_program								g_locale_table[ 140 ].value
#define ST_V_Hibernate									g_locale_table[ 141 ].value
#define ST_V_Hybrid_shut_down							g_locale_table[ 142 ].value
#define ST_V_Lock										g_locale_table[ 143 ].value
#define ST_V_Log_off									g_locale_table[ 144 ].value
#define ST_V_Move_deleted_downloads_to_Recycle_Bin		g_locale_table[ 145 ].value
#define ST_V_Move_files_that_match_category				g_locale_table[ 146 ].value
#define ST_V_None										g_locale_table[ 147 ].value
#define ST_V_Override_download_list_action_prompts		g_locale_table[ 148 ].value
#define ST_V_Overwrite_File								g_locale_table[ 149 ].value
#define ST_V_Prevent_system_standby						g_locale_table[ 150 ].value
#define ST_V_Rename_File								g_locale_table[ 151 ].value
#define ST_V_Restart_system								g_locale_table[ 152 ].value
#define ST_V_Restart_Download							g_locale_table[ 153 ].value
#define ST_V_Resume_previously_downloading				g_locale_table[ 154 ].value
#define ST_V_Set_date_and_time_of_file					g_locale_table[ 155 ].value
#define ST_V_Shut_down									g_locale_table[ 156 ].value
#define ST_V_Skip_Download								g_locale_table[ 157 ].value
#define ST_V_Sleep										g_locale_table[ 158 ].value
#define ST_V_System_shutdown_action_					g_locale_table[ 159 ].value
#define ST_V_Thread_pool_count_							g_locale_table[ 160 ].value
#define ST_V_Update_redirected_URL_s__in_download_list	g_locale_table[ 161 ].value
#define ST_V_Use_temporary_download_directory_			g_locale_table[ 162 ].value
#define ST_V_When_a_file_already_exists_				g_locale_table[ 163 ].value
#define ST_V_When_a_file_has_been_modified_				g_locale_table[ 164 ].value
#define ST_V_When_a_file_is_greater_than_or_equal_to_	g_locale_table[ 165 ].value

// Options Appearance
#define ST_V_Background_Color							g_locale_table[ 166 ].value
#define ST_V_Background_Font_Color						g_locale_table[ 167 ].value
#define ST_V_Border_Color								g_locale_table[ 168 ].value
#define ST_V_Download_list_								g_locale_table[ 169 ].value
#define ST_V_Draw_all_rows								g_locale_table[ 170 ].value
#define ST_V_Draw_full_rows								g_locale_table[ 171 ].value
#define ST_V_Even_Row_Background_Color					g_locale_table[ 172 ].value
#define ST_V_Even_Row_Font								g_locale_table[ 173 ].value
#define ST_V_Even_Row_Font_Color						g_locale_table[ 174 ].value
#define ST_V_Even_Row_Highlight_Color					g_locale_table[ 175 ].value
#define ST_V_Even_Row_Highlight_Font_Color				g_locale_table[ 176 ].value
#define ST_V_Expand_added_group_items					g_locale_table[ 177 ].value
#define ST_V_Gridline_Color								g_locale_table[ 178 ].value
#define ST_V_Odd_Row_Background_Color					g_locale_table[ 179 ].value
#define ST_V_Odd_Row_Font								g_locale_table[ 180 ].value
#define ST_V_Odd_Row_Font_Color							g_locale_table[ 181 ].value
#define ST_V_Odd_Row_Highlight_Color					g_locale_table[ 182 ].value
#define ST_V_Odd_Row_Highlight_Font_Color				g_locale_table[ 183 ].value
#define ST_V_Other_progress_bars_						g_locale_table[ 184 ].value
#define ST_V_Progress_Color								g_locale_table[ 185 ].value
#define ST_V_Progress_bar_								g_locale_table[ 186 ].value
#define ST_V_Progress_Font_Color						g_locale_table[ 187 ].value
#define ST_V_Scroll_to_last_item_when_adding_URL_s_		g_locale_table[ 188 ].value
#define ST_V_Selection_Marquee_Color					g_locale_table[ 189 ].value
#define ST_V_Show_executable_s_embedded_icon			g_locale_table[ 190 ].value
#define ST_V_Show_gridlines_in_download_list			g_locale_table[ 191 ].value
#define ST_V_Show_progress_for_each_part				g_locale_table[ 192 ].value
#define ST_V_Sort_added_and_updating_items				g_locale_table[ 193 ].value
#define ST_V_System_Tray_Icon_Downloading				g_locale_table[ 194 ].value
#define ST_V_System_Tray_Icon_Paused					g_locale_table[ 195 ].value
#define ST_V_System_Tray_Icon_Error						g_locale_table[ 196 ].value
#define ST_V_URL_Drop_Window_Downloading				g_locale_table[ 197 ].value
#define ST_V_URL_Drop_Window_Paused						g_locale_table[ 198 ].value
#define ST_V_URL_Drop_Window_Error						g_locale_table[ 199 ].value

// Options Connection
#define ST_V_Active_download_limit_						g_locale_table[ 200 ].value
#define ST_V_Default_download_parts_					g_locale_table[ 201 ].value
#define ST_V_Default_SSL___TLS_version_					g_locale_table[ 202 ].value
#define ST_V_Maximum_redirects_							g_locale_table[ 203 ].value
#define ST_V_Reallocate_parts_to_maximize_connections_	g_locale_table[ 204 ].value
#define ST_V_Reallocate_threshold_size__bytes__			g_locale_table[ 205 ].value
#define ST_V_Retry_incomplete_downloads_				g_locale_table[ 206 ].value
#define ST_V_Retry_incomplete_parts_					g_locale_table[ 207 ].value
#define ST_V_Timeout__seconds__							g_locale_table[ 208 ].value

// Options FTP
#define ST_V_DASH										g_locale_table[ 209 ].value
#define ST_V_Active										g_locale_table[ 210 ].value
#define ST_V_Active_Listen_Information					g_locale_table[ 211 ].value
#define ST_V_Data_Transfer_Mode							g_locale_table[ 212 ].value
#define ST_V_Passive									g_locale_table[ 213 ].value
#define ST_V_Port_end_									g_locale_table[ 214 ].value
#define ST_V_Port_start_								g_locale_table[ 215 ].value
#define ST_V_Send_keep_alive_requests					g_locale_table[ 216 ].value
#define ST_V_Use_other_mode_on_failure					g_locale_table[ 217 ].value

// Options General
#define ST_V_Always_on_top								g_locale_table[ 218 ].value
#define ST_V_Check_for_updates_upon_startup				g_locale_table[ 219 ].value
#define ST_V_Close_to_System_Tray						g_locale_table[ 220 ].value
#define ST_V_Enable_System_Tray_icon_					g_locale_table[ 221 ].value
#define ST_V_Enable_URL_drop_window_					g_locale_table[ 222 ].value
#define ST_V_Failure_									g_locale_table[ 223 ].value
#define ST_V_Load_Download_Finish_Sound_File			g_locale_table[ 224 ].value
#define ST_V_Minimize_to_System_Tray					g_locale_table[ 225 ].value
#define ST_V_Play_sound_when_downloads_finish			g_locale_table[ 226 ].value
#define ST_V_Show_notification_when_downloads_finish	g_locale_table[ 227 ].value
#define ST_V_Show_progress_bar							g_locale_table[ 228 ].value
#define ST_V_Start_in_System_Tray						g_locale_table[ 229 ].value
#define ST_V_Success_									g_locale_table[ 230 ].value
#define ST_V_Transparency_								g_locale_table[ 231 ].value
#define ST_V_PLAY										g_locale_table[ 232 ].value

// Options Proxy
#define ST_V_Allow_proxy_to_resolve_domain_names		g_locale_table[ 233 ].value
#define ST_V_Allow_proxy_to_resolve_domain_names_v4a	g_locale_table[ 234 ].value
#define ST_V_Hostname_									g_locale_table[ 235 ].value
#define ST_V_SOCKS_v4									g_locale_table[ 236 ].value
#define ST_V_SOCKS_v5									g_locale_table[ 237 ].value
#define ST_V_Use_Authentication_						g_locale_table[ 238 ].value
#define ST_V_Use_HTTP_proxy_							g_locale_table[ 239 ].value
#define ST_V_Use_HTTPS_proxy_							g_locale_table[ 240 ].value
#define ST_V_Use_SOCKS_proxy_							g_locale_table[ 241 ].value

// Options Server
#define ST_V_COLON										g_locale_table[ 242 ].value
#define ST_V_Basic_Authentication						g_locale_table[ 243 ].value
#define ST_V_Certificate_file_							g_locale_table[ 244 ].value
#define ST_V_Digest_Authentication						g_locale_table[ 245 ].value
#define ST_V_Enable_server_								g_locale_table[ 246 ].value
#define ST_V_Enable_SSL___TLS_							g_locale_table[ 247 ].value
#define ST_V_Hostname___IPv6_address_					g_locale_table[ 248 ].value
#define ST_V_IPv4_address_								g_locale_table[ 249 ].value
#define ST_V_Key_file_									g_locale_table[ 250 ].value
#define ST_V_Load_PKCS_NUM12_File						g_locale_table[ 251 ].value
#define ST_V_Load_Private_Key_File						g_locale_table[ 252 ].value
#define ST_V_Load_X_509_Certificate_File				g_locale_table[ 253 ].value
#define ST_V_PKCS_NUM12_								g_locale_table[ 254 ].value
#define ST_V_PKCS_NUM12_file_							g_locale_table[ 255 ].value
#define ST_V_PKCS_NUM12_password_						g_locale_table[ 256 ].value
#define ST_V_Port_										g_locale_table[ 257 ].value
#define ST_V_Public___Private_key_pair_					g_locale_table[ 258 ].value
#define ST_V_Require_authentication_					g_locale_table[ 259 ].value
#define ST_V_Server										g_locale_table[ 260 ].value
#define ST_V_Server_SSL___TLS_version_					g_locale_table[ 261 ].value
#define ST_V_Show_notification_for_remote_connections	g_locale_table[ 262 ].value

// Options SFTP
#define ST_V_Algorithm_Selection_Policies				g_locale_table[ 263 ].value
#define ST_V_Attempt_GSSAPI_authentication				g_locale_table[ 264 ].value
#define ST_V_Attempt_GSSAPI_key_exchange				g_locale_table[ 265 ].value
#define ST_V_Drag_items_to_reorder_priority				g_locale_table[ 266 ].value
#define ST_V_Enable_compression							g_locale_table[ 267 ].value
#define ST_V_Encryption_cipher_							g_locale_table[ 268 ].value
#define ST_V_Fingerprint								g_locale_table[ 269 ].value
#define ST_V_Fingerprint_								g_locale_table[ 270 ].value
#define ST_V_Fingerprints_								g_locale_table[ 271 ].value
#define ST_V_GSS_rekey_time__minutes__					g_locale_table[ 272 ].value
#define ST_V_Host										g_locale_table[ 273 ].value
#define ST_V_Host_										g_locale_table[ 274 ].value
#define ST_V_Host_key_									g_locale_table[ 275 ].value
#define ST_V_Host_Key_Algorithm							g_locale_table[ 276 ].value
#define ST_V_Host_key_algorithm_						g_locale_table[ 277 ].value
#define ST_V_Key_Group_exchange_						g_locale_table[ 278 ].value
#define ST_V_Load_Private_Key							g_locale_table[ 279 ].value
#define ST_V_Private_Key_File							g_locale_table[ 280 ].value
#define ST_V_Private_key_file_							g_locale_table[ 281 ].value
#define ST_V_Rekey_data_limit__bytes__					g_locale_table[ 282 ].value
#define ST_V_Rekey_time__minutes__						g_locale_table[ 283 ].value
#define ST_V_Send_keep_alive_requests__seconds__		g_locale_table[ 284 ].value

// CMessageBox
#define ST_V_Continue									g_locale_table[ 285 ].value
#define ST_V_No											g_locale_table[ 286 ].value
#define ST_V_Overwrite									g_locale_table[ 287 ].value
#define ST_V_Remember_choice							g_locale_table[ 288 ].value
#define ST_V_Skip										g_locale_table[ 289 ].value
#define ST_V_Skip_remaining_messages					g_locale_table[ 290 ].value
#define ST_V_Yes										g_locale_table[ 291 ].value

// Add URL(s)
#define ST_V_Advanced_options							g_locale_table[ 292 ].value
#define	ST_V_Authentication								g_locale_table[ 293 ].value
#define ST_V_Cookies									g_locale_table[ 294 ].value
#define ST_V_Cookies_									g_locale_table[ 295 ].value
#define ST_V_Custom_Filter								g_locale_table[ 296 ].value
#define ST_V_Download									g_locale_table[ 297 ].value
#define ST_V_Download_directory_						g_locale_table[ 298 ].value
#define ST_V_Download_parts_							g_locale_table[ 299 ].value
#define ST_V_Headers									g_locale_table[ 300 ].value
#define ST_V_Headers_									g_locale_table[ 301 ].value
#define ST_V_Images_Filter								g_locale_table[ 302 ].value
#define ST_V_Music_Filter								g_locale_table[ 303 ].value
#define ST_V_Password_									g_locale_table[ 304 ].value
#define ST_V_POST_Data									g_locale_table[ 305 ].value
#define ST_V_RegEx_filter_								g_locale_table[ 306 ].value
#define ST_V_Send_POST_Data_							g_locale_table[ 307 ].value
#define ST_V_Simulate_download							g_locale_table[ 308 ].value
#define ST_V_SSL___TLS_version_							g_locale_table[ 309 ].value
#define ST_V_URL_s__									g_locale_table[ 310 ].value
#define ST_V_Use_proxy_									g_locale_table[ 311 ].value
#define ST_V_Username_									g_locale_table[ 312 ].value
#define ST_V_Verify										g_locale_table[ 313 ].value
#define ST_V_Videos_Filter								g_locale_table[ 314 ].value

// Search
#define ST_V_Match_case									g_locale_table[ 315 ].value
#define ST_V_Match_whole_word							g_locale_table[ 316 ].value
#define ST_V_Regular_expression							g_locale_table[ 317 ].value
#define ST_V_Search										g_locale_table[ 318 ].value
#define ST_V_Search_All									g_locale_table[ 319 ].value
#define ST_V_Search_for_								g_locale_table[ 320 ].value
#define ST_V_Search_Next								g_locale_table[ 321 ].value
#define ST_V_Search_Type								g_locale_table[ 322 ].value

// Site Manager
#define ST_V__PASSWORD_									g_locale_table[ 323 ].value
#define ST_V__DATA_										g_locale_table[ 324 ].value
#define ST_V_Add										g_locale_table[ 325 ].value
#define ST_V_Close										g_locale_table[ 326 ].value
#define ST_V_Disable									g_locale_table[ 327 ].value
#define ST_V_Download_operation_						g_locale_table[ 328 ].value
#define ST_V_Download_Operations						g_locale_table[ 329 ].value
#define ST_V_Download_Parts								g_locale_table[ 330 ].value
#define ST_V_Empty_Body									g_locale_table[ 331 ].value
#define ST_V_Enable										g_locale_table[ 332 ].value
#define ST_V_New										g_locale_table[ 333 ].value
#define ST_V_Password									g_locale_table[ 334 ].value
#define ST_V_Proxy_Password								g_locale_table[ 335 ].value
#define ST_V_Proxy_Port									g_locale_table[ 336 ].value
#define ST_V_Proxy_Server								g_locale_table[ 337 ].value
#define ST_V_Proxy_Type									g_locale_table[ 338 ].value
#define ST_V_Proxy_Username								g_locale_table[ 339 ].value
#define ST_V_Resolve_Domain_Names						g_locale_table[ 340 ].value
#define ST_V_Save										g_locale_table[ 341 ].value
#define ST_V_Show_passwords								g_locale_table[ 342 ].value
#define ST_V_Simulate									g_locale_table[ 343 ].value
#define ST_V_Site										g_locale_table[ 344 ].value
#define ST_V_Site_										g_locale_table[ 345 ].value
#define ST_V_Username									g_locale_table[ 346 ].value

// Add Category
#define ST_V_Associate_file_extension_s__with_category_	g_locale_table[ 347 ].value

// Fingerprint Prompt
#define ST_V_Add_host_and_key_information_to_cache		g_locale_table[ 348 ].value
#define ST_V_Key_size_									g_locale_table[ 349 ].value

// Update Check
#define ST_V_A_new_version_is_available_				g_locale_table[ 350 ].value
#define ST_V_Checking_for_updates___					g_locale_table[ 351 ].value
#define ST_V_Current_version_							g_locale_table[ 352 ].value
#define ST_V_Download_Update							g_locale_table[ 353 ].value
#define ST_V_HTTP_Downloader_is_up_to_date_				g_locale_table[ 354 ].value
#define ST_V_Latest_version_							g_locale_table[ 355 ].value
#define ST_V_The_update_check_has_failed_				g_locale_table[ 356 ].value
#define ST_V_View_Changelog								g_locale_table[ 357 ].value
#define ST_V_Visit_Home_Page							g_locale_table[ 358 ].value

// Common
#define ST_V_BTN___										g_locale_table[ 359 ].value
#define ST_V__Simulated_								g_locale_table[ 360 ].value
#define ST_V_Add_Category								g_locale_table[ 361 ].value
#define ST_V_Add_URL_s_									g_locale_table[ 362 ].value
#define ST_V_Added										g_locale_table[ 363 ].value
#define ST_V_All										g_locale_table[ 364 ].value
#define ST_V_Allocating_File							g_locale_table[ 365 ].value
#define ST_V_Authorization_Required						g_locale_table[ 366 ].value
#define ST_V_Cancel										g_locale_table[ 367 ].value
#define ST_V_Categories									g_locale_table[ 368 ].value
#define ST_V_Category_									g_locale_table[ 369 ].value
#define ST_V_Check_For_Updates							g_locale_table[ 370 ].value
#define ST_V_Comments_									g_locale_table[ 371 ].value
#define ST_V_Completed									g_locale_table[ 372 ].value
#define ST_V_Connecting									g_locale_table[ 373 ].value
#define ST_V_Default									g_locale_table[ 374 ].value
#define ST_V_Default_download_speed_limit_				g_locale_table[ 375 ].value
#define ST_V_Desktop									g_locale_table[ 376 ].value
#define ST_V_Documents									g_locale_table[ 377 ].value
#define ST_V_Download_speed_							g_locale_table[ 378 ].value
#define ST_V_Download_speed_limit_bytes_				g_locale_table[ 379 ].value
#define ST_V_Downloading								g_locale_table[ 380 ].value
#define ST_V_Downloads									g_locale_table[ 381 ].value
#define ST_V_Downloads_Have_Finished					g_locale_table[ 382 ].value
#define ST_V_Error										g_locale_table[ 383 ].value
#define ST_V_Export_Download_History					g_locale_table[ 384 ].value
#define ST_V_Failed										g_locale_table[ 385 ].value
#define ST_V_File_IO_Error								g_locale_table[ 386 ].value
#define ST_V_Global_Download_Speed_Limit				g_locale_table[ 387 ].value
#define ST_V_Global_download_speed_limit_				g_locale_table[ 388 ].value
#define ST_V_Global_download_speed_limit_bytes_			g_locale_table[ 389 ].value
#define ST_V_HTTP										g_locale_table[ 390 ].value
#define ST_V_HTTPS										g_locale_table[ 391 ].value
#define ST_V_Import_Download_History					g_locale_table[ 392 ].value
#define ST_V_Insufficient_Disk_Space					g_locale_table[ 393 ].value
#define ST_V_IP_address_								g_locale_table[ 394 ].value
#define ST_V_Items_										g_locale_table[ 395 ].value
#define ST_V_Moving_File								g_locale_table[ 396 ].value
#define ST_V_Music										g_locale_table[ 397 ].value
#define ST_V_Options									g_locale_table[ 398 ].value
#define ST_V_Other										g_locale_table[ 399 ].value
#define ST_V_Paused										g_locale_table[ 400 ].value
#define ST_V_Pictures									g_locale_table[ 401 ].value
#define ST_V_Proxy_Authentication_Required				g_locale_table[ 402 ].value
#define ST_V_Queued										g_locale_table[ 403 ].value
#define ST_V_Remote_Connection							g_locale_table[ 404 ].value
#define ST_V_Restarting									g_locale_table[ 405 ].value
#define ST_V_Save_Download_History						g_locale_table[ 406 ].value
#define ST_V_Set										g_locale_table[ 407 ].value
#define ST_V_Site_Manager								g_locale_table[ 408 ].value
#define ST_V_Skipped									g_locale_table[ 409 ].value
#define ST_V_Status										g_locale_table[ 410 ].value
#define ST_V_Stopped									g_locale_table[ 411 ].value
#define ST_V_SSL_2_0									g_locale_table[ 412 ].value
#define ST_V_SSL_3_0									g_locale_table[ 413 ].value
#define ST_V_Timed_Out									g_locale_table[ 414 ].value
#define ST_V_TLS_1_0									g_locale_table[ 415 ].value
#define ST_V_TLS_1_1									g_locale_table[ 416 ].value
#define ST_V_TLS_1_2									g_locale_table[ 417 ].value
#define ST_V_TLS_1_3									g_locale_table[ 418 ].value
#define ST_V_Total_downloaded_							g_locale_table[ 419 ].value
#define ST_V_Unlimited									g_locale_table[ 420 ].value
#define ST_V_Update										g_locale_table[ 421 ].value
#define ST_V_Update_Category							g_locale_table[ 422 ].value
#define ST_V_Update_Download							g_locale_table[ 423 ].value
#define ST_V_URL_										g_locale_table[ 424 ].value
#define ST_V_URL_s__added_								g_locale_table[ 425 ].value
#define ST_V_Videos										g_locale_table[ 426 ].value

// Common Messages
#define ST_V_A_key_algorithm_must_be_supplied			g_locale_table[ 427 ].value
#define ST_V_A_private_key_file_is_required				g_locale_table[ 428 ].value
#define ST_V_A_protocol_must_be_supplied				g_locale_table[ 429 ].value
#define ST_V_A_restart_is_required						g_locale_table[ 430 ].value
#define ST_V_A_restart_is_required_disable_allocation	g_locale_table[ 431 ].value
#define ST_V_A_restart_is_required_enable_allocation	g_locale_table[ 432 ].value
#define ST_V_A_restart_is_required_shutdown				g_locale_table[ 433 ].value
#define ST_V_A_restart_is_required_threads				g_locale_table[ 434 ].value
#define ST_V_PROMPT_delete_selected_files				g_locale_table[ 435 ].value
#define ST_V_PROMPT_remove_completed_entries			g_locale_table[ 436 ].value
#define ST_V_PROMPT_remove_and_delete_selected_entries	g_locale_table[ 437 ].value
#define ST_V_PROMPT_remove_selected_entries				g_locale_table[ 438 ].value
#define ST_V_PROMPT_restart_selected_entries			g_locale_table[ 439 ].value
#define ST_V_PROMPT_accept_the_server_host_key			g_locale_table[ 440 ].value
#define ST_V_Item_ID_List_was_not_freed					g_locale_table[ 441 ].value
#define ST_V_One_or_more_file_extensions_exist			g_locale_table[ 442 ].value
#define ST_V_One_or_more_files_are_in_use				g_locale_table[ 443 ].value
#define ST_V_One_or_more_files_were_not_found			g_locale_table[ 444 ].value
#define ST_V_One_or_more_paths_were_not_found			g_locale_table[ 445 ].value
#define ST_V_Select_the_category_download_directory		g_locale_table[ 446 ].value
#define ST_V_Select_the_default_download_directory		g_locale_table[ 447 ].value
#define ST_V_Select_the_download_directory				g_locale_table[ 448 ].value
#define ST_V_Select_the_temporary_download_directory	g_locale_table[ 449 ].value
#define ST_V_The_download_will_be_resumed				g_locale_table[ 450 ].value
#define ST_V_File_is_in_use_cannot_delete				g_locale_table[ 451 ].value
#define ST_V_File_is_in_use_cannot_rename				g_locale_table[ 452 ].value
#define ST_V_File_format_is_incorrect					g_locale_table[ 453 ].value
#define ST_V_PROMPT_mismatch_accept_the_server_host_key	g_locale_table[ 454 ].value
#define ST_V_The_specified_category_already_exists		g_locale_table[ 455 ].value
#define ST_V_The_specified_file_was_not_found			g_locale_table[ 456 ].value
#define ST_V_PROMPT_The_specified_file_was_not_found	g_locale_table[ 457 ].value
#define ST_V_The_specified_host_already_exists			g_locale_table[ 458 ].value
#define ST_V_The_specified_host_is_invalid				g_locale_table[ 459 ].value
#define ST_V_The_specified_path_was_not_found			g_locale_table[ 460 ].value
#define ST_V_The_specified_site_already_exists			g_locale_table[ 461 ].value
#define ST_V_The_specified_site_is_invalid				g_locale_table[ 462 ].value
#define ST_V_The_specified_un_and_host_already_exists	g_locale_table[ 463 ].value
#define ST_V_There_is_already_a_file					g_locale_table[ 464 ].value
#define ST_V_You_must_supply_download_directory			g_locale_table[ 465 ].value

// About
#define ST_V_BETA										g_locale_table[ 466 ].value
#define ST_V_BUILT										g_locale_table[ 467 ].value
#define ST_V_COPYRIGHT									g_locale_table[ 468 ].value
#define ST_V_LICENSE									g_locale_table[ 469 ].value
#define ST_V_VERSION									g_locale_table[ 470 ].value

// Dynamic Messages
#define ST_V_PROMPT___already_exists					g_locale_table[ 471 ].value
#define ST_V_PROMPT___could_not_be_renamed				g_locale_table[ 472 ].value
#define ST_V_PROMPT___has_been_modified					g_locale_table[ 473 ].value
#define ST_V_PROMPT___will_be___size					g_locale_table[ 474 ].value

// SFTP KEX Algorithms
#define ST_V_DH_Group1_SHA1								g_locale_table[ 475 ].value
#define ST_V_DH_Group14_SHA1							g_locale_table[ 476 ].value
#define ST_V_DH_GEX_SHA1								g_locale_table[ 477 ].value
#define ST_V_RSA_KEX									g_locale_table[ 478 ].value
#define ST_V_ECDH										g_locale_table[ 479 ].value

// SFTP Host Key Algorithms
#define ST_V_RSA										g_locale_table[ 480 ].value
#define ST_V_DSA										g_locale_table[ 481 ].value
#define ST_V_ECDSA										g_locale_table[ 482 ].value
#define ST_V_Ed25519									g_locale_table[ 483 ].value

// SFTP Encryption Ciphers
#define ST_V_3DES										g_locale_table[ 484 ].value
#define ST_V_Blowfish									g_locale_table[ 485 ].value
#define ST_V_AES										g_locale_table[ 486 ].value
#define ST_V_DES										g_locale_table[ 487 ].value
#define ST_V_Arcfour									g_locale_table[ 488 ].value
#define ST_V_ChaCha20									g_locale_table[ 489 ].value

// SFTP Host Key Algorithms (cached)
#define ST_V_ecdsa_sha2_nistp256						g_locale_table[ 490 ].value
#define ST_V_ecdsa_sha2_nistp384						g_locale_table[ 491 ].value
#define ST_V_ecdsa_sha2_nistp521						g_locale_table[ 492 ].value
#define ST_V_ssh_dss									g_locale_table[ 493 ].value
#define ST_V_ssh_ed25519								g_locale_table[ 494 ].value
#define ST_V_ssh_rsa									g_locale_table[ 495 ].value

// File Filters
#define ST_V_All_Files									g_locale_table[ 496 ].value
#define ST_V_CSV__Comma_delimited_						g_locale_table[ 497 ].value
#define ST_V_Download_History							g_locale_table[ 498 ].value
#define ST_V_Personal_Information_Exchange				g_locale_table[ 499 ].value
//#define ST_V_Private_Keys								g_locale_table[ 500 ].value
#define ST_V_PuTTY_Private_Key_Files					g_locale_table[ 501 ].value
#define ST_V_WAV										g_locale_table[ 502 ].value
#define ST_V_X_509_Certificates							g_locale_table[ 503 ].value

//

// Month
#define ST_L_January									g_locale_table[ 0 ].length
#define ST_L_February									g_locale_table[ 1 ].length
#define ST_L_March										g_locale_table[ 2 ].length
#define ST_L_April										g_locale_table[ 3 ].length
#define ST_L_May										g_locale_table[ 4 ].length
#define ST_L_June										g_locale_table[ 5 ].length
#define ST_L_July										g_locale_table[ 6 ].length
#define ST_L_August										g_locale_table[ 7 ].length
#define ST_L_September									g_locale_table[ 8 ].length
#define ST_L_October									g_locale_table[ 9 ].length
#define ST_L_November									g_locale_table[ 10 ].length
#define ST_L_December									g_locale_table[ 11 ].length

// Day
#define ST_L_Sunday										g_locale_table[ 12 ].length
#define ST_L_Monday										g_locale_table[ 13 ].length
#define ST_L_Tuesday									g_locale_table[ 14 ].length
#define ST_L_Wednesday									g_locale_table[ 15 ].length
#define ST_L_Thursday									g_locale_table[ 16 ].length
#define ST_L_Friday										g_locale_table[ 17 ].length
#define ST_L_Saturday									g_locale_table[ 18 ].length
/*
// Download
#define ST_L_NUM										g_locale_table[ 19 ].length
#define ST_L_Active_Parts								g_locale_table[ 20 ].length
#define ST_L_Category									g_locale_table[ 21 ].length
#define ST_L_Comments									g_locale_table[ 22 ].length
#define ST_L_Date_and_Time_Added						g_locale_table[ 23 ].length
#define ST_L_Download_Directory							g_locale_table[ 24 ].length
#define ST_L_Download_Speed								g_locale_table[ 25 ].length
#define ST_L_Download_Speed_Limit						g_locale_table[ 26 ].length
#define ST_L_Downloaded									g_locale_table[ 27 ].length
#define ST_L_File_Size									g_locale_table[ 28 ].length
#define ST_L_File_Type									g_locale_table[ 29 ].length
#define ST_L_Filename									g_locale_table[ 30 ].length
#define ST_L_Progress									g_locale_table[ 31 ].length
#define ST_L_SSL___TLS_Version							g_locale_table[ 32 ].length
#define ST_L_Time_Elapsed								g_locale_table[ 33 ].length
#define ST_L_Time_Remaining								g_locale_table[ 34 ].length
#define ST_L_URL										g_locale_table[ 35 ].length
*/
// Menu
#define ST_L_NUM										g_locale_table[ 36 ].length
#define ST_L__About										g_locale_table[ 37 ].length
#define ST_L_Active_Parts								g_locale_table[ 38 ].length
#define ST_L_Add_Category___							g_locale_table[ 39 ].length
#define ST_L__Add_URL_s____								g_locale_table[ 40 ].length
#define ST_L_Add_URL_s____								g_locale_table[ 41 ].length
#define ST_L_Always_on_Top								g_locale_table[ 42 ].length
#define ST_L_C_ategories								g_locale_table[ 43 ].length
#define ST_L_Category									g_locale_table[ 44 ].length
#define ST_L__Check_for_Updates___						g_locale_table[ 45 ].length
#define ST_L__Column_Headers							g_locale_table[ 46 ].length
#define ST_L_Comments									g_locale_table[ 47 ].length
#define ST_L__Copy_URL_s_								g_locale_table[ 48 ].length
#define ST_L_Copy_URL_s_								g_locale_table[ 49 ].length
#define ST_L_Date_and_Time_Added						g_locale_table[ 50 ].length
#define ST_L__Delete_									g_locale_table[ 51 ].length
#define ST_L_Delete										g_locale_table[ 52 ].length
#define ST_L_Download_Directory							g_locale_table[ 53 ].length
#define ST_L_Download_Speed								g_locale_table[ 54 ].length
#define ST_L_Download_Speed_Limit						g_locale_table[ 55 ].length
#define ST_L_Downloaded									g_locale_table[ 56 ].length
#define ST_L__Edit										g_locale_table[ 57 ].length
#define ST_L_Enable_List__Edit_Mode						g_locale_table[ 58 ].length
#define ST_L_Enable_List_Edit_Mode						g_locale_table[ 59 ].length
#define ST_L_E_xit										g_locale_table[ 60 ].length
#define ST_L_Exit										g_locale_table[ 61 ].length
#define ST_L__Export_Download_History___				g_locale_table[ 62 ].length
#define ST_L__File										g_locale_table[ 63 ].length
#define ST_L_File_Size									g_locale_table[ 64 ].length
#define ST_L_File_Type									g_locale_table[ 65 ].length
#define ST_L_Filename									g_locale_table[ 66 ].length
#define ST_L_Global_Download_Speed__Limit____			g_locale_table[ 67 ].length
#define ST_L__Help										g_locale_table[ 68 ].length
#define ST_L_HTTP_Downloader__Home_Page					g_locale_table[ 69 ].length
#define ST_L__Import_Download_History___				g_locale_table[ 70 ].length
#define ST_L_Move_Down									g_locale_table[ 71 ].length
#define ST_L_Move_to_Bottom								g_locale_table[ 72 ].length
#define ST_L_Move_to_Top								g_locale_table[ 73 ].length
#define ST_L_Move_Up									g_locale_table[ 74 ].length
#define ST_L_Open_Directory								g_locale_table[ 75 ].length
#define ST_L_Open_File									g_locale_table[ 76 ].length
#define ST_L_Open_Download_List							g_locale_table[ 77 ].length
#define ST_L__Options____								g_locale_table[ 78 ].length
#define ST_L_Options___									g_locale_table[ 79 ].length
#define ST_L__Pause										g_locale_table[ 80 ].length
#define ST_L_Pause										g_locale_table[ 81 ].length
#define ST_L_Pause_Active								g_locale_table[ 82 ].length
#define ST_L_Progress									g_locale_table[ 83 ].length
#define ST_L_Queue										g_locale_table[ 84 ].length
#define ST_L__Remove_									g_locale_table[ 85 ].length
#define ST_L_Remove										g_locale_table[ 86 ].length
#define ST_L_Remove_and_Delete_							g_locale_table[ 87 ].length
#define ST_L_Remove_and_Delete							g_locale_table[ 88 ].length
#define ST_L_Remove_Completed_							g_locale_table[ 89 ].length
#define ST_L_Rename_									g_locale_table[ 90 ].length
#define ST_L_Rename										g_locale_table[ 91 ].length
#define ST_L_Restart									g_locale_table[ 92 ].length
#define ST_L_Resu_me									g_locale_table[ 93 ].length
#define ST_L_Resume										g_locale_table[ 94 ].length
#define ST_L__Save_Download_History___					g_locale_table[ 95 ].length
#define ST_L__Search____								g_locale_table[ 96 ].length
#define ST_L__Select_All_								g_locale_table[ 97 ].length
#define ST_L_Select_All									g_locale_table[ 98 ].length
#define ST_L_Site__Manager____							g_locale_table[ 99 ].length
#define ST_L_SSL___TLS_Version							g_locale_table[ 100 ].length
#define ST_L_St_art										g_locale_table[ 101 ].length
#define ST_L_Start										g_locale_table[ 102 ].length
#define ST_L_Start___Resume_Inactive					g_locale_table[ 103 ].length
#define ST_L__Status_Bar								g_locale_table[ 104 ].length
#define ST_L_St_op										g_locale_table[ 105 ].length
#define ST_L_Stop										g_locale_table[ 106 ].length
#define ST_L_Stop_All									g_locale_table[ 107 ].length
#define ST_L_Time_Elapsed								g_locale_table[ 108 ].length
#define ST_L_Time_Remaining								g_locale_table[ 109 ].length
#define ST_L__Toolbar									g_locale_table[ 110 ].length
#define ST_L__Tools										g_locale_table[ 111 ].length
#define ST_L_Update_Category___							g_locale_table[ 112 ].length
#define ST_L__Update_Download____						g_locale_table[ 113 ].length
#define ST_L_Update_Download___							g_locale_table[ 114 ].length
#define ST_L_URL										g_locale_table[ 115 ].length
#define ST_L__View										g_locale_table[ 116 ].length

// Options
#define ST_L_Advanced									g_locale_table[ 117 ].length
#define ST_L_Appearance									g_locale_table[ 118 ].length
#define ST_L_Apply										g_locale_table[ 119 ].length
#define ST_L_Connection									g_locale_table[ 120 ].length
#define ST_L_Fingerprints								g_locale_table[ 121 ].length
#define ST_L_FTP										g_locale_table[ 122 ].length
#define ST_L_General									g_locale_table[ 123 ].length
#define ST_L_OK											g_locale_table[ 124 ].length
#define ST_L_Private_Keys								g_locale_table[ 125 ].length
#define ST_L_Proxy										g_locale_table[ 126 ].length
#define ST_L_SFTP										g_locale_table[ 127 ].length

// Options Advanced
#define ST_L_Add_in_Stopped_state						g_locale_table[ 128 ].length
#define ST_L_Allow_only_one_instance					g_locale_table[ 129 ].length
#define ST_L_Apply_initially_set_proxy					g_locale_table[ 130 ].length
#define ST_L_Continue_Download							g_locale_table[ 131 ].length
#define ST_L_Default_download_directory_				g_locale_table[ 132 ].length
#define ST_L_Display_Prompt								g_locale_table[ 133 ].length
#define ST_L_Download_immediately						g_locale_table[ 134 ].length
#define ST_L_Download_non_200_and_non_206_responses		g_locale_table[ 135 ].length
#define ST_L_Drag_and_drop_URL_s__action_				g_locale_table[ 136 ].length
#define ST_L_Enable_download_history					g_locale_table[ 137 ].length
#define ST_L_Enable_quick_file_allocation				g_locale_table[ 138 ].length
#define ST_L_Enable_sparse_file_allocation				g_locale_table[ 139 ].length
#define ST_L_Exit_program								g_locale_table[ 140 ].length
#define ST_L_Hibernate									g_locale_table[ 141 ].length
#define ST_L_Hybrid_shut_down							g_locale_table[ 142 ].length
#define ST_L_Lock										g_locale_table[ 143 ].length
#define ST_L_Log_off									g_locale_table[ 144 ].length
#define ST_L_Move_deleted_downloads_to_Recycle_Bin		g_locale_table[ 145 ].length
#define ST_L_Move_files_that_match_category				g_locale_table[ 146 ].length
#define ST_L_None										g_locale_table[ 147 ].length
#define ST_L_Override_download_list_action_prompts		g_locale_table[ 148 ].length
#define ST_L_Overwrite_File								g_locale_table[ 149 ].length
#define ST_L_Prevent_system_standby						g_locale_table[ 150 ].length
#define ST_L_Rename_File								g_locale_table[ 151 ].length
#define ST_L_Restart_system								g_locale_table[ 152 ].length
#define ST_L_Restart_Download							g_locale_table[ 153 ].length
#define ST_L_Resume_previously_downloading				g_locale_table[ 154 ].length
#define ST_L_Set_date_and_time_of_file					g_locale_table[ 155 ].length
#define ST_L_Shut_down									g_locale_table[ 156 ].length
#define ST_L_Skip_Download								g_locale_table[ 157 ].length
#define ST_L_Sleep										g_locale_table[ 158 ].length
#define ST_L_System_shutdown_action_					g_locale_table[ 159 ].length
#define ST_L_Thread_pool_count_							g_locale_table[ 160 ].length
#define ST_L_Update_redirected_URL_s__in_download_list	g_locale_table[ 161 ].length
#define ST_L_Use_temporary_download_directory_			g_locale_table[ 162 ].length
#define ST_L_When_a_file_already_exists_				g_locale_table[ 163 ].length
#define ST_L_When_a_file_has_been_modified_				g_locale_table[ 164 ].length
#define ST_L_When_a_file_is_greater_than_or_equal_to_	g_locale_table[ 165 ].length

// Options Appearance
#define ST_L_Background_Color							g_locale_table[ 166 ].length
#define ST_L_Background_Font_Color						g_locale_table[ 167 ].length
#define ST_L_Border_Color								g_locale_table[ 168 ].length
#define ST_L_Download_list_								g_locale_table[ 169 ].length
#define ST_L_Draw_all_rows								g_locale_table[ 170 ].length
#define ST_L_Draw_full_rows								g_locale_table[ 171 ].length
#define ST_L_Even_Row_Background_Color					g_locale_table[ 172 ].length
#define ST_L_Even_Row_Font								g_locale_table[ 173 ].length
#define ST_L_Even_Row_Font_Color						g_locale_table[ 174 ].length
#define ST_L_Even_Row_Highlight_Color					g_locale_table[ 175 ].length
#define ST_L_Even_Row_Highlight_Font_Color				g_locale_table[ 176 ].length
#define ST_L_Expand_added_group_items					g_locale_table[ 177 ].length
#define ST_L_Gridline_Color								g_locale_table[ 178 ].length
#define ST_L_Odd_Row_Background_Color					g_locale_table[ 179 ].length
#define ST_L_Odd_Row_Font								g_locale_table[ 180 ].length
#define ST_L_Odd_Row_Font_Color							g_locale_table[ 181 ].length
#define ST_L_Odd_Row_Highlight_Color					g_locale_table[ 182 ].length
#define ST_L_Odd_Row_Highlight_Font_Color				g_locale_table[ 183 ].length
#define ST_L_Other_progress_bars_						g_locale_table[ 184 ].length
#define ST_L_Progress_Color								g_locale_table[ 185 ].length
#define ST_L_Progress_bar_								g_locale_table[ 186 ].length
#define ST_L_Progress_Font_Color						g_locale_table[ 187 ].length
#define ST_L_Scroll_to_last_item_when_adding_URL_s_		g_locale_table[ 188 ].length
#define ST_L_Selection_Marquee_Color					g_locale_table[ 189 ].length
#define ST_L_Show_executable_s_embedded_icon			g_locale_table[ 190 ].length
#define ST_L_Show_gridlines_in_download_list			g_locale_table[ 191 ].length
#define ST_L_Show_progress_for_each_part				g_locale_table[ 192 ].length
#define ST_L_Sort_added_and_updating_items				g_locale_table[ 193 ].length
#define ST_L_System_Tray_Icon_Downloading				g_locale_table[ 194 ].length
#define ST_L_System_Tray_Icon_Paused					g_locale_table[ 195 ].length
#define ST_L_System_Tray_Icon_Error						g_locale_table[ 196 ].length
#define ST_L_URL_Drop_Window_Downloading				g_locale_table[ 197 ].length
#define ST_L_URL_Drop_Window_Paused						g_locale_table[ 198 ].length
#define ST_L_URL_Drop_Window_Error						g_locale_table[ 199 ].length

// Options Connection
#define ST_L_Active_download_limit_						g_locale_table[ 200 ].length
#define ST_L_Default_download_parts_					g_locale_table[ 201 ].length
#define ST_L_Default_SSL___TLS_version_					g_locale_table[ 202 ].length
#define ST_L_Maximum_redirects_							g_locale_table[ 203 ].length
#define ST_L_Reallocate_parts_to_maximize_connections_	g_locale_table[ 204 ].length
#define ST_L_Reallocate_threshold_size__bytes__			g_locale_table[ 205 ].length
#define ST_L_Retry_incomplete_downloads_				g_locale_table[ 206 ].length
#define ST_L_Retry_incomplete_parts_					g_locale_table[ 207 ].length
#define ST_L_Timeout__seconds__							g_locale_table[ 208 ].length

// Options FTP
#define ST_L_DASH										g_locale_table[ 209 ].length
#define ST_L_Active										g_locale_table[ 210 ].length
#define ST_L_Active_Listen_Information					g_locale_table[ 211 ].length
#define ST_L_Data_Transfer_Mode							g_locale_table[ 212 ].length
#define ST_L_Passive									g_locale_table[ 213 ].length
#define ST_L_Port_end_									g_locale_table[ 214 ].length
#define ST_L_Port_start_								g_locale_table[ 215 ].length
#define ST_L_Send_keep_alive_requests					g_locale_table[ 216 ].length
#define ST_L_Use_other_mode_on_failure					g_locale_table[ 217 ].length

// Options General
#define ST_L_Always_on_top								g_locale_table[ 218 ].length
#define ST_L_Check_for_updates_upon_startup				g_locale_table[ 219 ].length
#define ST_L_Close_to_System_Tray						g_locale_table[ 220 ].length
#define ST_L_Enable_System_Tray_icon_					g_locale_table[ 221 ].length
#define ST_L_Enable_URL_drop_window_					g_locale_table[ 222 ].length
#define ST_L_Failure_									g_locale_table[ 223 ].length
#define ST_L_Load_Download_Finish_Sound_File			g_locale_table[ 224 ].length
#define ST_L_Minimize_to_System_Tray					g_locale_table[ 225 ].length
#define ST_L_Play_sound_when_downloads_finish			g_locale_table[ 226 ].length
#define ST_L_Show_notification_when_downloads_finish	g_locale_table[ 227 ].length
#define ST_L_Show_progress_bar							g_locale_table[ 228 ].length
#define ST_L_Start_in_System_Tray						g_locale_table[ 229 ].length
#define ST_L_Success_									g_locale_table[ 230 ].length
#define ST_L_Transparency_								g_locale_table[ 231 ].length
#define ST_L_PLAY										g_locale_table[ 232 ].length

// Options Proxy
#define ST_L_Allow_proxy_to_resolve_domain_names		g_locale_table[ 233 ].length
#define ST_L_Allow_proxy_to_resolve_domain_names_v4a	g_locale_table[ 234 ].length
#define ST_L_Hostname_									g_locale_table[ 235 ].length
#define ST_L_SOCKS_v4									g_locale_table[ 236 ].length
#define ST_L_SOCKS_v5									g_locale_table[ 237 ].length
#define ST_L_Use_Authentication_						g_locale_table[ 238 ].length
#define ST_L_Use_HTTP_proxy_							g_locale_table[ 239 ].length
#define ST_L_Use_HTTPS_proxy_							g_locale_table[ 240 ].length
#define ST_L_Use_SOCKS_proxy_							g_locale_table[ 241 ].length

// Options Server
#define ST_L_COLON										g_locale_table[ 242 ].length
#define ST_L_Basic_Authentication						g_locale_table[ 243 ].length
#define ST_L_Certificate_file_							g_locale_table[ 244 ].length
#define ST_L_Digest_Authentication						g_locale_table[ 245 ].length
#define ST_L_Enable_server_								g_locale_table[ 246 ].length
#define ST_L_Enable_SSL___TLS_							g_locale_table[ 247 ].length
#define ST_L_Hostname___IPv6_address_					g_locale_table[ 248 ].length
#define ST_L_IPv4_address_								g_locale_table[ 249 ].length
#define ST_L_Key_file_									g_locale_table[ 250 ].length
#define ST_L_Load_PKCS_NUM12_File						g_locale_table[ 251 ].length
#define ST_L_Load_Private_Key_File						g_locale_table[ 252 ].length
#define ST_L_Load_X_509_Certificate_File				g_locale_table[ 253 ].length
#define ST_L_PKCS_NUM12_								g_locale_table[ 254 ].length
#define ST_L_PKCS_NUM12_file_							g_locale_table[ 255 ].length
#define ST_L_PKCS_NUM12_password_						g_locale_table[ 256 ].length
#define ST_L_Port_										g_locale_table[ 257 ].length
#define ST_L_Public___Private_key_pair_					g_locale_table[ 258 ].length
#define ST_L_Require_authentication_					g_locale_table[ 259 ].length
#define ST_L_Server										g_locale_table[ 260 ].length
#define ST_L_Server_SSL___TLS_version_					g_locale_table[ 261 ].length
#define ST_L_Show_notification_for_remote_connections	g_locale_table[ 262 ].length

// Options SFTP
#define ST_L_Algorithm_Selection_Policies				g_locale_table[ 263 ].length
#define ST_L_Attempt_GSSAPI_authentication				g_locale_table[ 264 ].length
#define ST_L_Attempt_GSSAPI_key_exchange				g_locale_table[ 265 ].length
#define ST_L_Drag_items_to_reorder_priority				g_locale_table[ 266 ].length
#define ST_L_Enable_compression							g_locale_table[ 267 ].length
#define ST_L_Encryption_cipher_							g_locale_table[ 268 ].length
#define ST_L_Fingerprint								g_locale_table[ 269 ].length
#define ST_L_Fingerprint_								g_locale_table[ 270 ].length
#define ST_L_Fingerprints_								g_locale_table[ 271 ].length
#define ST_L_GSS_rekey_time__minutes__					g_locale_table[ 272 ].length
#define ST_L_Host										g_locale_table[ 273 ].length
#define ST_L_Host_										g_locale_table[ 274 ].length
#define ST_L_Host_key_									g_locale_table[ 275 ].length
#define ST_L_Host_Key_Algorithm							g_locale_table[ 276 ].length
#define ST_L_Host_key_algorithm_						g_locale_table[ 277 ].length
#define ST_L_Key_Group_exchange_						g_locale_table[ 278 ].length
#define ST_L_Load_Private_Key							g_locale_table[ 279 ].length
#define ST_L_Private_Key_File							g_locale_table[ 280 ].length
#define ST_L_Private_key_file_							g_locale_table[ 281 ].length
#define ST_L_Rekey_data_limit__bytes__					g_locale_table[ 282 ].length
#define ST_L_Rekey_time__minutes__						g_locale_table[ 283 ].length
#define ST_L_Send_keep_alive_requests__seconds__		g_locale_table[ 284 ].length

// CMessageBox
#define ST_L_Continue									g_locale_table[ 285 ].length
#define ST_L_No											g_locale_table[ 286 ].length
#define ST_L_Overwrite									g_locale_table[ 287 ].length
#define ST_L_Remember_choice							g_locale_table[ 288 ].length
#define ST_L_Skip										g_locale_table[ 289 ].length
#define ST_L_Skip_remaining_messages					g_locale_table[ 290 ].length
#define ST_L_Yes										g_locale_table[ 291 ].length

// Add URL(s)
#define ST_L_Advanced_options							g_locale_table[ 292 ].length
#define	ST_L_Authentication								g_locale_table[ 293 ].length
#define ST_L_Cookies									g_locale_table[ 294 ].length
#define ST_L_Cookies_									g_locale_table[ 295 ].length
#define ST_L_Custom_Filter								g_locale_table[ 296 ].length
#define ST_L_Download									g_locale_table[ 297 ].length
#define ST_L_Download_directory_						g_locale_table[ 298 ].length
#define ST_L_Download_parts_							g_locale_table[ 299 ].length
#define ST_L_Headers									g_locale_table[ 300 ].length
#define ST_L_Headers_									g_locale_table[ 301 ].length
#define ST_L_Images_Filter								g_locale_table[ 302 ].length
#define ST_L_Music_Filter								g_locale_table[ 303 ].length
#define ST_L_Password_									g_locale_table[ 304 ].length
#define ST_L_POST_Data									g_locale_table[ 305 ].length
#define ST_L_RegEx_filter_								g_locale_table[ 306 ].length
#define ST_L_Send_POST_Data_							g_locale_table[ 307 ].length
#define ST_L_Simulate_download							g_locale_table[ 308 ].length
#define ST_L_SSL___TLS_version_							g_locale_table[ 309 ].length
#define ST_L_URL_s__									g_locale_table[ 310 ].length
#define ST_L_Use_proxy_									g_locale_table[ 311 ].length
#define ST_L_Username_									g_locale_table[ 312 ].length
#define ST_L_Verify										g_locale_table[ 313 ].length
#define ST_L_Videos_Filter								g_locale_table[ 314 ].length

// Search
#define ST_L_Match_case									g_locale_table[ 315 ].length
#define ST_L_Match_whole_word							g_locale_table[ 316 ].length
#define ST_L_Regular_expression							g_locale_table[ 317 ].length
#define ST_L_Search										g_locale_table[ 318 ].length
#define ST_L_Search_All									g_locale_table[ 319 ].length
#define ST_L_Search_for_								g_locale_table[ 320 ].length
#define ST_L_Search_Next								g_locale_table[ 321 ].length
#define ST_L_Search_Type								g_locale_table[ 322 ].length

// Site Manager
#define ST_L__PASSWORD_									g_locale_table[ 323 ].length
#define ST_L__DATA_										g_locale_table[ 324 ].length
#define ST_L_Add										g_locale_table[ 325 ].length
#define ST_L_Close										g_locale_table[ 326 ].length
#define ST_L_Disable									g_locale_table[ 327 ].length
#define ST_L_Download_operation_						g_locale_table[ 328 ].length
#define ST_L_Download_Operations						g_locale_table[ 329 ].length
#define ST_L_Download_Parts								g_locale_table[ 330 ].length
#define ST_L_Empty_Body									g_locale_table[ 331 ].length
#define ST_L_Enable										g_locale_table[ 332 ].length
#define ST_L_New										g_locale_table[ 333 ].length
#define ST_L_Password									g_locale_table[ 334 ].length
#define ST_L_Proxy_Password								g_locale_table[ 335 ].length
#define ST_L_Proxy_Port									g_locale_table[ 336 ].length
#define ST_L_Proxy_Server								g_locale_table[ 337 ].length
#define ST_L_Proxy_Type									g_locale_table[ 338 ].length
#define ST_L_Proxy_Username								g_locale_table[ 339 ].length
#define ST_L_Resolve_Domain_Names						g_locale_table[ 340 ].length
#define ST_L_Save										g_locale_table[ 341 ].length
#define ST_L_Show_passwords								g_locale_table[ 342 ].length
#define ST_L_Simulate									g_locale_table[ 343 ].length
#define ST_L_Site										g_locale_table[ 344 ].length
#define ST_L_Site_										g_locale_table[ 345 ].length
#define ST_L_Username									g_locale_table[ 346 ].length

// Add Category
#define ST_L_Associate_file_extension_s__with_category_	g_locale_table[ 347 ].length

// Fingerprint Prompt
#define ST_L_Add_host_and_key_information_to_cache		g_locale_table[ 348 ].length
#define ST_L_Key_size_									g_locale_table[ 349 ].length

// Update Check
#define ST_L_A_new_version_is_available_				g_locale_table[ 350 ].length
#define ST_L_Checking_for_updates___					g_locale_table[ 351 ].length
#define ST_L_Current_version_							g_locale_table[ 352 ].length
#define ST_L_Download_Update							g_locale_table[ 353 ].length
#define ST_L_HTTP_Downloader_is_up_to_date_				g_locale_table[ 354 ].length
#define ST_L_Latest_version_							g_locale_table[ 355 ].length
#define ST_L_The_update_check_has_failed_				g_locale_table[ 356 ].length
#define ST_L_View_Changelog								g_locale_table[ 357 ].length
#define ST_L_Visit_Home_Page							g_locale_table[ 358 ].length

// Common
#define ST_L_BTN___										g_locale_table[ 359 ].length
#define ST_L__Simulated_								g_locale_table[ 360 ].length
#define ST_L_Add_Category								g_locale_table[ 361 ].length
#define ST_L_Add_URL_s_									g_locale_table[ 362 ].length
#define ST_L_Added										g_locale_table[ 363 ].length
#define ST_L_All										g_locale_table[ 364 ].length
#define ST_L_Allocating_File							g_locale_table[ 365 ].length
#define ST_L_Authorization_Required						g_locale_table[ 366 ].length
#define ST_L_Cancel										g_locale_table[ 367 ].length
#define ST_L_Categories									g_locale_table[ 368 ].length
#define ST_L_Category_									g_locale_table[ 369 ].length
#define ST_L_Check_For_Updates							g_locale_table[ 370 ].length
#define ST_L_Comments_									g_locale_table[ 371 ].length
#define ST_L_Completed									g_locale_table[ 372 ].length
#define ST_L_Connecting									g_locale_table[ 373 ].length
#define ST_L_Default									g_locale_table[ 374 ].length
#define ST_L_Default_download_speed_limit_				g_locale_table[ 375 ].length
#define ST_L_Desktop									g_locale_table[ 376 ].length
#define ST_L_Documents									g_locale_table[ 377 ].length
#define ST_L_Download_speed_							g_locale_table[ 378 ].length
#define ST_L_Download_speed_limit_bytes_				g_locale_table[ 379 ].length
#define ST_L_Downloading								g_locale_table[ 380 ].length
#define ST_L_Downloads									g_locale_table[ 381 ].length
#define ST_L_Downloads_Have_Finished					g_locale_table[ 382 ].length
#define ST_L_Error										g_locale_table[ 383 ].length
#define ST_L_Export_Download_History					g_locale_table[ 384 ].length
#define ST_L_Failed										g_locale_table[ 385 ].length
#define ST_L_File_IO_Error								g_locale_table[ 386 ].length
#define ST_L_Global_Download_Speed_Limit				g_locale_table[ 387 ].length
#define ST_L_Global_download_speed_limit_				g_locale_table[ 388 ].length
#define ST_L_Global_download_speed_limit_bytes_			g_locale_table[ 389 ].length
#define ST_L_HTTP										g_locale_table[ 390 ].length
#define ST_L_HTTPS										g_locale_table[ 391 ].length
#define ST_L_Import_Download_History					g_locale_table[ 392 ].length
#define ST_L_Insufficient_Disk_Space					g_locale_table[ 393 ].length
#define ST_L_IP_address_								g_locale_table[ 394 ].length
#define ST_L_Items_										g_locale_table[ 395 ].length
#define ST_L_Moving_File								g_locale_table[ 396 ].length
#define ST_L_Music										g_locale_table[ 397 ].length
#define ST_L_Options									g_locale_table[ 398 ].length
#define ST_L_Other										g_locale_table[ 399 ].length
#define ST_L_Paused										g_locale_table[ 400 ].length
#define ST_L_Pictures									g_locale_table[ 401 ].length
#define ST_L_Proxy_Authentication_Required				g_locale_table[ 402 ].length
#define ST_L_Queued										g_locale_table[ 403 ].length
#define ST_L_Remote_Connection							g_locale_table[ 404 ].length
#define ST_L_Restarting									g_locale_table[ 405 ].length
#define ST_L_Save_Download_History						g_locale_table[ 406 ].length
#define ST_L_Set										g_locale_table[ 407 ].length
#define ST_L_Site_Manager								g_locale_table[ 408 ].length
#define ST_L_Skipped									g_locale_table[ 409 ].length
#define ST_L_Status										g_locale_table[ 410 ].length
#define ST_L_Stopped									g_locale_table[ 411 ].length
#define ST_L_SSL_2_0									g_locale_table[ 412 ].length
#define ST_L_SSL_3_0									g_locale_table[ 413 ].length
#define ST_L_Timed_Out									g_locale_table[ 414 ].length
#define ST_L_TLS_1_0									g_locale_table[ 415 ].length
#define ST_L_TLS_1_1									g_locale_table[ 416 ].length
#define ST_L_TLS_1_2									g_locale_table[ 417 ].length
#define ST_L_TLS_1_3									g_locale_table[ 418 ].length
#define ST_L_Total_downloaded_							g_locale_table[ 419 ].length
#define ST_L_Unlimited									g_locale_table[ 420 ].length
#define ST_L_Update										g_locale_table[ 421 ].length
#define ST_L_Update_Category							g_locale_table[ 422 ].length
#define ST_L_Update_Download							g_locale_table[ 423 ].length
#define ST_L_URL_										g_locale_table[ 424 ].length
#define ST_L_URL_s__added_								g_locale_table[ 425 ].length
#define ST_L_Videos										g_locale_table[ 426 ].length

// Common Messages
#define ST_L_A_key_algorithm_must_be_supplied			g_locale_table[ 427 ].length
#define ST_L_A_private_key_file_is_required				g_locale_table[ 428 ].length
#define ST_L_A_protocol_must_be_supplied				g_locale_table[ 429 ].length
#define ST_L_A_restart_is_required						g_locale_table[ 430 ].length
#define ST_L_A_restart_is_required_disable_allocation	g_locale_table[ 431 ].length
#define ST_L_A_restart_is_required_enable_allocation	g_locale_table[ 432 ].length
#define ST_L_A_restart_is_required_shutdown				g_locale_table[ 433 ].length
#define ST_L_A_restart_is_required_threads				g_locale_table[ 434 ].length
#define ST_L_PROMPT_delete_selected_files				g_locale_table[ 435 ].length
#define ST_L_PROMPT_remove_completed_entries			g_locale_table[ 436 ].length
#define ST_L_PROMPT_remove_and_delete_selected_entries	g_locale_table[ 437 ].length
#define ST_L_PROMPT_remove_selected_entries				g_locale_table[ 438 ].length
#define ST_L_PROMPT_restart_selected_entries			g_locale_table[ 439 ].length
#define ST_L_PROMPT_accept_the_server_host_key			g_locale_table[ 440 ].length
#define ST_L_Item_ID_List_was_not_freed					g_locale_table[ 441 ].length
#define ST_L_One_or_more_file_extensions_exist			g_locale_table[ 442 ].length
#define ST_L_One_or_more_files_are_in_use				g_locale_table[ 443 ].length
#define ST_L_One_or_more_files_were_not_found			g_locale_table[ 444 ].length
#define ST_L_One_or_more_paths_were_not_found			g_locale_table[ 445 ].length
#define ST_L_Select_the_category_download_directory		g_locale_table[ 446 ].length
#define ST_L_Select_the_default_download_directory		g_locale_table[ 447 ].length
#define ST_L_Select_the_download_directory				g_locale_table[ 448 ].length
#define ST_L_Select_the_temporary_download_directory	g_locale_table[ 449 ].length
#define ST_L_The_download_will_be_resumed				g_locale_table[ 450 ].length
#define ST_L_File_is_in_use_cannot_delete				g_locale_table[ 451 ].length
#define ST_L_File_is_in_use_cannot_rename				g_locale_table[ 452 ].length
#define ST_L_File_format_is_incorrect					g_locale_table[ 453 ].length
#define ST_L_PROMPT_mismatch_accept_the_server_host_key	g_locale_table[ 454 ].length
#define ST_L_The_specified_category_already_exists		g_locale_table[ 455 ].length
#define ST_L_The_specified_file_was_not_found			g_locale_table[ 456 ].length
#define ST_L_PROMPT_The_specified_file_was_not_found	g_locale_table[ 457 ].length
#define ST_L_The_specified_host_already_exists			g_locale_table[ 458 ].length
#define ST_L_The_specified_host_is_invalid				g_locale_table[ 459 ].length
#define ST_L_The_specified_path_was_not_found			g_locale_table[ 460 ].length
#define ST_L_The_specified_site_already_exists			g_locale_table[ 461 ].length
#define ST_L_The_specified_site_is_invalid				g_locale_table[ 462 ].length
#define ST_L_The_specified_un_and_host_already_exists	g_locale_table[ 463 ].length
#define ST_L_There_is_already_a_file					g_locale_table[ 464 ].length
#define ST_L_You_must_supply_download_directory			g_locale_table[ 465 ].length

// About
#define ST_L_BETA										g_locale_table[ 466 ].length
#define ST_L_BUILT										g_locale_table[ 467 ].length
#define ST_L_COPYRIGHT									g_locale_table[ 468 ].length
#define ST_L_LICENSE									g_locale_table[ 469 ].length
#define ST_L_VERSION									g_locale_table[ 470 ].length

// Dynamic Messages
#define ST_L_PROMPT___already_exists					g_locale_table[ 471 ].length
#define ST_L_PROMPT___could_not_be_renamed				g_locale_table[ 472 ].length
#define ST_L_PROMPT___has_been_modified					g_locale_table[ 473 ].length
#define ST_L_PROMPT___will_be___size					g_locale_table[ 474 ].length

// SFTP KEX Algorithms
#define ST_L_DH_Group1_SHA1								g_locale_table[ 475 ].length
#define ST_L_DH_Group14_SHA1							g_locale_table[ 476 ].length
#define ST_L_DH_GEX_SHA1								g_locale_table[ 477 ].length
#define ST_L_RSA_KEX									g_locale_table[ 478 ].length
#define ST_L_ECDH										g_locale_table[ 479 ].length

// SFTP Host Key Algorithms
#define ST_L_RSA										g_locale_table[ 480 ].length
#define ST_L_DSA										g_locale_table[ 481 ].length
#define ST_L_ECDSA										g_locale_table[ 482 ].length
#define ST_L_Ed25519									g_locale_table[ 483 ].length

// SFTP Encryption Ciphers
#define ST_L_3DES										g_locale_table[ 484 ].length
#define ST_L_Blowfish									g_locale_table[ 485 ].length
#define ST_L_AES										g_locale_table[ 486 ].length
#define ST_L_DES										g_locale_table[ 487 ].length
#define ST_L_Arcfour									g_locale_table[ 488 ].length
#define ST_L_ChaCha20									g_locale_table[ 489 ].length

// SFTP Host Key Algorithms (cached)
#define ST_L_ecdsa_sha2_nistp256						g_locale_table[ 490 ].length
#define ST_L_ecdsa_sha2_nistp384						g_locale_table[ 491 ].length
#define ST_L_ecdsa_sha2_nistp521						g_locale_table[ 492 ].length
#define ST_L_ssh_dss									g_locale_table[ 493 ].length
#define ST_L_ssh_ed25519								g_locale_table[ 494 ].length
#define ST_L_ssh_rsa									g_locale_table[ 495 ].length

// File Filters
#define ST_L_All_Files									g_locale_table[ 496 ].length
#define ST_L_CSV__Comma_delimited_						g_locale_table[ 497 ].length
#define ST_L_Download_History							g_locale_table[ 498 ].length
#define ST_L_Personal_Information_Exchange				g_locale_table[ 499 ].length
//#define ST_L_Private_Keys								g_locale_table[ 500 ].length
#define ST_L_PuTTY_Private_Key_Files					g_locale_table[ 501 ].length
#define ST_L_WAV										g_locale_table[ 502 ].length
#define ST_L_X_509_Certificates							g_locale_table[ 503 ].length

#endif
