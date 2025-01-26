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

#ifndef _CATEGORIES
#define _CATEGORIES

#define MENU_CAT_ADD			50000
#define MENU_CAT_UPDATE			50001
#define MENU_CAT_REMOVE			50002
#define MENU_CAT_OPEN			50003

struct SHARED_CATEGORY_INFO
{
	wchar_t *category;
	unsigned int count;
};

struct CATEGORY_INFO_
{
	wchar_t *category;
	wchar_t *file_extensions;
	wchar_t *download_directory;
};

struct CATEGORY_FILE_EXTENSION_INFO
{
	CATEGORY_INFO_ *ci;
	wchar_t *file_extension;
};

struct CATEGORY_UPDATE_INFO
{
	CATEGORY_INFO_ *ci;
	CATEGORY_INFO_ *old_ci;
	HTREEITEM hti;
	unsigned char update_type;	// 0 = Add, 1 = Update, 2 = Remove
};

#define CATEGORY_TREE_INFO_TYPE_NONE			0
#define CATEGORY_TREE_INFO_TYPE_STATUS			1
#define CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO	2

struct CATEGORY_TREE_INFO
{
	void *data;
	unsigned char type;	// 0 = None, 1 = Status, 2 = CATEGORY_INFO_
};

extern HMENU g_hMenuSub_categories_add;
extern HMENU g_hMenuSub_categories_update_remove;

extern HTREEITEM g_hti_categories;

void CreateCategoryTreeView( HWND hWnd_categories );

char read_category_info();
char save_category_info();

THREAD_RETURN load_category_list( void *pArguments );
void FreeCategoryInfo( CATEGORY_INFO_ **category_info );
int dllrbt_compare_category_info( void *a, void *b );

THREAD_RETURN load_window_category_list( void *pArguments );
THREAD_RETURN handle_category_list( void *pArguments );
THREAD_RETURN handle_category_move( void *pArguments );

void RefreshSelectedFilter( unsigned int status, wchar_t *category );

void CleanupCategoryList();

extern DoublyLinkedList *g_treeview_list;
extern DoublyLinkedList *g_category_list;
extern bool category_list_changed;

extern HWND g_hWnd_au_category;
extern HWND g_hWnd_sm_category;
extern HWND g_hWnd_update_category;

extern bool g_update_add_category_window;
extern bool g_update_update_category_window;
extern bool g_update_site_manager_category_window;

extern dllrbt_tree *g_shared_categories;
extern dllrbt_tree *g_category_file_extensions;

wchar_t *CacheCategory( wchar_t *category );
wchar_t *RemoveCachedCategory( wchar_t *category );

LRESULT CALLBACK CategoriesEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern WNDPROC CategoriesEditProc;

extern CATEGORY_TREE_INFO *g_drag_and_drop_cti;

#define CATEGORY_STATUS	( UINT_MAX - 1 )

#endif
