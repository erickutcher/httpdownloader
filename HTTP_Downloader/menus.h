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

#ifndef _MENUS_H
#define _MENUS_H

#define MENU_START					10000
#define MENU_PAUSE					10001
#define MENU_STOP					10002

#define MENU_RESTART				10003

#define MENU_REMOVE					10004
#define MENU_REMOVE_AND_DELETE		10005

#define MENU_COPY_URLS				10006

#define	MENU_SELECT_ALL				10007

#define MENU_START_INACTIVE			10008
#define MENU_PAUSE_ACTIVE			10009
#define MENU_STOP_ALL				10010
#define MENU_REMOVE_COMPLETED		10011
#define MENU_DELETE					10012
#define MENU_RENAME					10013

#define MENU_OPEN_FILE				10014
#define MENU_OPEN_DIRECTORY			10015

#define MENU_UPDATE_DOWNLOAD		10016

#define MENU_LIST_EDIT_MODE			10017

#define MENU_QUEUE_TOP				10100
#define MENU_QUEUE_UP				10101
#define MENU_QUEUE_DOWN				10102
#define MENU_QUEUE_BOTTOM			10103

#define MENU_COLUMNS				20000

#define MENU_NUM					20000
#define MENU_ACTIVE_PARTS			20001
#define MENU_CATEGORY				20002
#define MENU_COMMENTS				20003
#define MENU_DATE_AND_TIME_ADDED	20004
#define MENU_DOWNLOAD_DIRECTORY		20005
#define MENU_DOWNLOAD_SPEED			20006
#define MENU_DOWNLOAD_SPEED_LIMIT	20007
#define MENU_DOWNLOADED				20008
#define MENU_FILE_SIZE				20009
#define MENU_FILE_TYPE				20010
#define MENU_FILENAME				20011
#define MENU_PROGRESS				20012
#define MENU_SSL_TLS_VERSION		20013
#define MENU_TIME_ELAPSED			20014
#define MENU_TIME_REMAINING			20015
#define MENU_URL					20016

#define COLUMN_MENU_OFFSET			20000

#define MENU_ADD_URLS				30000
#define MENU_SAVE_DOWNLOAD_HISTORY	30001
#define MENU_IMPORT_DOWNLOAD_HISTORY	30002
#define MENU_EXPORT_DOWNLOAD_HISTORY	30003
#define MENU_EXIT					30004

#define MENU_SHOW_TOOLBAR			30005
#define MENU_SHOW_CATEGORIES		30006
#define MENU_SHOW_COLUMN_HEADERS	30007
#define MENU_SHOW_STATUS_BAR		30008

#define MENU_SEARCH					30009
#define MENU_GLOBAL_SPEED_LIMIT		30010
#define MENU_SITE_MANAGER			30011
#define MENU_OPTIONS				30012

#define	MENU_HOME_PAGE				30013
#define MENU_CHECK_FOR_UPDATES		30014
#define MENU_ABOUT					30015

#define MENU_ALWAYS_ON_TOP			30016

#define MENU_RESTORE				40000

void DestroyMenus();
void UpdateMenus( bool enable );
void CreateMenus();

void UpdateColumns( WORD menu_id );
void HandleCommand( HWND hWnd, WORD command );

extern HMENU g_hMenu;
extern HMENU g_hMenuSub_view;

extern HMENU g_hMenuSub_download;
extern HMENU g_hMenuSub_column;
extern HMENU g_hMenuSub_tray;
extern HMENU g_hMenuSub_drag_drop;

#endif
