/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#define MENU_REMOVE_SELECTED		10003

#define MENU_COPY_URLS				10004

#define	MENU_SELECT_ALL				10005

#define MENU_PAUSE_ACTIVE			10006
#define MENU_STOP_ALL				10007
#define MENU_REMOVE_COMPLETED		10008

#define MENU_OPEN_FILE				10009
#define MENU_OPEN_DIRECTORY			10010

#define MENU_UPDATE_DOWNLOAD		10011

#define MENU_QUEUE_TOP				10100
#define MENU_QUEUE_UP				10101
#define MENU_QUEUE_DOWN				10102
#define MENU_QUEUE_BOTTOM			10103

#define MENU_NUM					20000
#define MENU_ACTIVE_PARTS			20001
#define MENU_DATE_AND_TIME_ADDED	20002
#define MENU_DOWNLOAD_DIRECTORY		20003
#define MENU_DOWNLOAD_SPEED			20004
#define MENU_DOWNLOADED				20005
#define MENU_FILE_SIZE				20006
#define MENU_FILE_TYPE				20007
#define MENU_FILENAME				20008
#define MENU_PROGRESS				20009
#define MENU_TIME_ELAPSED			20010
#define MENU_TIME_REMAINING			20011
#define MENU_TLS_SSL_VERSION		20012
#define MENU_URL					20013

#define COLUMN_MENU_OFFSET			20000

#define MENU_ADD_URLS				30000
#define MENU_EXIT					30001

#define MENU_SHOW_STATUS_BAR		30002

#define MENU_OPTIONS				30003

#define	MENU_HOME_PAGE				30004
#define MENU_ABOUT					30005

#define MENU_RESTORE				40000

void DestroyMenus();
void UpdateMenus( bool enable );
void CreateMenus();

void UpdateColumns( unsigned int menu_id );

extern HMENU g_hMenu;
extern HMENU g_hMenuSub_view;

extern HMENU g_hMenuSub_download;
extern HMENU g_hMenuSub_column;
extern HMENU g_hMenuSub_tray;

#endif
