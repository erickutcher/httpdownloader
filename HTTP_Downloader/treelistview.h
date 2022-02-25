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

#ifndef _TREELISTVIEW_H
#define _TREELISTVIEW_H

#include "windows.h"

#define TLVM_REFRESH_LIST			( WM_APP + 1 )
#define TLVM_EDIT_LABEL				( WM_APP + 2 )
#define TLVM_SH_COLUMN_HEADERS		( WM_APP + 3 )
#define TLVM_TOGGLE_DRAW			( WM_APP + 4 )
#define TLVM_SORT_ITEMS				( WM_APP + 5 )

////

// States

#define TLVS_NONE					0x00
#define TLVS_SELECTED				0x01
#define TLVS_FOCUSED				0x02

////

// Data Types

#define TLVDT_GROUP					0x01
#define TLVDT_HOST					0x02

struct TREELISTNODE;

struct TREELISTNODE
{
	TREELISTNODE *next;
	TREELISTNODE *prev;
	TREELISTNODE *parent;
	TREELISTNODE *child;

	void *data;

	int child_count;

	unsigned char data_type;
	unsigned char flag;
	
	bool is_expanded;
};

//int TLV_CountChildren( TREELISTNODE *tln, bool allow_collapsed );
int TLV_GetTotalItemCount();
int TLV_SetTotalItemCount( int total_item_count );
int TLV_GetExpandedItemCount();
int TLV_SetExpandedItemCount( int expanded_item_count );
int TLV_GetRootItemCount();
int TLV_SetRootItemCount( int root_item_count );
int TLV_GetSelectedCount();
int TLV_SetSelectedCount( int selected_count );

void TLV_ClearSelected( bool ctrl_down, bool shift_down );

void TLV_ResetSelectionBounds();
void TLV_SetSelectionBounds( int index, TREELISTNODE *tln );

TREELISTNODE *TLV_GetFocusedItem();
void TLV_SetFocusedItem( TREELISTNODE *tln );
int TLV_GetFocusedIndex();
int TLV_SetFocusedIndex( int index );
int TLV_GetNextSelectedItem( TREELISTNODE *start_node, int start_index, TREELISTNODE **ret_node/*, bool top_level = false*/ );

int TLV_GetParentIndex( TREELISTNODE *tln, int index );

TREELISTNODE *TLV_GetFirstSelectedItem();
TREELISTNODE *TLV_GetLastSelectedItem();
int TLV_GetFirstSelectedIndex();
int TLV_GetLastSelectedIndex();

int TLV_GetFirstVisibleIndex();
void TLV_SetFirstVisibleIndex( int first_visible_index );
TREELISTNODE *TLV_GetFirstVisibleItem();
void TLV_SetFirstVisibleItem( TREELISTNODE *tln );

int TLV_GetFirstVisibleRootIndex();
void TLV_SetFirstVisibleRootIndex( int first_visible_root_index );

int TLV_GetVisibleItemCount();

void TLV_SelectAll( HWND hWnd, bool allow_collapsed );

TREELISTNODE *TLV_PrevNode( TREELISTNODE *node, bool allow_collapsed );
TREELISTNODE *TLV_NextNode( TREELISTNODE *node, bool allow_collapsed );

void TLV_AddNode( TREELISTNODE **head, TREELISTNODE *node, int position );
void TLV_RemoveNode( TREELISTNODE **head, TREELISTNODE *node );

int TLV_FreeTree( TREELISTNODE *tln );

LRESULT CALLBACK TreeListViewWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

extern TREELISTNODE *g_tree_list;

extern TREELISTNODE *g_focused_node;
extern int g_focused_index;

extern TREELISTNODE *g_base_selected_node;
extern int g_base_selected_index;

extern TREELISTNODE *g_first_visible_node;
extern int g_first_visible_index;

extern int g_drag_start_index;

extern int g_expanded_item_count;
extern int g_visible_item_count;
extern int g_total_item_count;

extern int g_selected_count;

extern TREELISTNODE *g_first_selection_node;
extern TREELISTNODE *g_last_selection_node;
extern int g_first_selection_index;	// When multiple items are selected, this is the first one in the list.
extern int g_last_selection_index;	// When multiple items are selected, this is the last one in the list.

extern HWND g_hWnd_tlv_header;

extern int g_header_width;

//

extern bool g_in_list_edit_mode;

//

#endif
