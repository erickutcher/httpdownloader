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

#include "utilities.h"
#include "string_tables.h"

#include "lite_shell32.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"
#include "lite_ole32.h"

#include "menus.h"

#include "treelistview.h"
#include "categories.h"
#include "doublylinkedlist.h"

#include "connection.h"
#include "list_operations.h"

#include "drag_and_drop.h"

#include "dark_mode.h"

#define EDIT_BOX	1000

#define IDT_SCROLL_TIMER	10000
#define IDT_EDIT_TIMER		10001

#define SCROLL_TYPE_UP		1
#define SCROLL_TYPE_DOWN	2
#define SCROLL_TYPE_LEFT	3
#define SCROLL_TYPE_RIGHT	4

//

#define DT_BUFFER_LENGTH	1024
#define ELLIPSISW_LENGTH	3
const static wchar_t *ELLIPSISW = L"...";

#define _DT_LEFT	0x00000000
#define _DT_CENTER	0x00000001
#define _DT_RIGHT	0x00000002
#define _DT_VCENTER	0x00000004

//

void HandleMouseMovement( HWND hWnd );
void HandleMouseDrag( HWND hWnd );

bool g_scroll_timer_active = false;
unsigned char g_v_scroll_direction = 0;	// 0 no scroll, 1 = down, 2 = up
unsigned char g_h_scroll_direction = 0;	// 0 no scroll, 1 = left, 2 = right

HWND g_hWnd_tlv_header = NULL;
HWND g_hWnd_tlv_tooltip = NULL;
HWND g_hWnd_edit_box = NULL;

WNDPROC EditBoxProc = NULL;

int g_header_width = 0;
int g_header_height = 0;

int g_row_height = 0;

int g_visible_rows = 0;

int hs_step = 4;

#define SCROLL_AMOUNT			3
#define SCROLL_TIMER_FREQUENCY	30

bool g_is_dragging = false;
RECT g_drag_rc;
POINT g_drag_pos;

RECT g_client_rc;

HBITMAP g_hbm = NULL;
bool g_size_changed = false;

bool g_skip_window_change = false;

RECT g_edit_column_rc;

unsigned char g_show_edit_state = 0;		// 0 = Not activated/showing, 1 = Activating (wait GetDoubleClickTime), 2 = Activated/Showing

unsigned char g_v_scroll_line_amount = 1;
unsigned char g_h_scroll_line_amount = 1;

HTHEME g_hTheme = NULL;

#define GLYPH_OFFSET		2

LONG g_glyph_offset = GLYPH_OFFSET;
SIZE g_glyph_size;

FONT_SETTINGS tlv_odd_row_font_settings;
FONT_SETTINGS tlv_even_row_font_settings;

///////////////////
wchar_t g_typing_buf[ MAX_PATH ];
unsigned short g_typing_buf_offset = 0;

#define TYPING_DELAY	450
DWORD g_last_typing_time = 0;

TREELISTNODE *g_search_start_node = NULL;
//////////////////

TREELISTNODE *g_tree_list = NULL;

TREELISTNODE *g_focused_node = NULL;		// Is the last selected node that was clicked or was dragged to.
int g_focused_index = -1;

TREELISTNODE *g_base_selected_node = NULL;	// The starting node of a click or drag.
int g_base_selected_index = -1;

TREELISTNODE *g_first_visible_node = NULL;
int g_first_visible_index = 0;
int g_first_visible_root_index = 0;

volatile LONG g_visible_item_count = 0;		// Number of items that are visible in the window.
volatile LONG g_total_item_count = 0;		// All items in the list (both expanded and contracted).
volatile LONG g_root_item_count = 0;		// Total number of parents.
volatile LONG g_expanded_item_count = 0;	// Parents + children of expanded parents.

int g_drag_start_index = -1;

volatile LONG g_total_parent_item_nodes = 0;

////////////

TREELISTNODE *g_first_selection_node = NULL;
TREELISTNODE *g_last_selection_node = NULL;
int g_first_selection_index = -1;			// When multiple items are selected, this is the first one in the list.
int g_last_selection_index = -1;			// When multiple items are selected, this is the last one in the list.

////////////

bool g_mod_key_active = false;

int g_mod_first_selection_index = -1;
TREELISTNODE *g_mod_first_selection_node = NULL;

int g_mod_last_selection_index = -1;
TREELISTNODE *g_mod_last_selection_node = NULL;

////////////

volatile LONG g_selected_count = 0;

////////////

wchar_t *g_tlv_tooltip_buffer = NULL;
int g_tlv_last_tooltip_item = -1;				// Prevent our hot tracking from calling the tooltip on the same item.

BOOL g_tlv_is_tracking = FALSE;
int g_tracking_x = -1;
int g_tracking_y = -1;

////////////

// Dragging items.

bool g_in_list_edit_mode = false;

bool g_draw_drag = false;

////////////

// Drag and Drop from TreeListView.

bool g_tlv_is_drag_and_drop = false;

////////////

// Filter items.

unsigned int g_status_filter = STATUS_NONE;
wchar_t *g_category_filter = NULL;
volatile LONG g_refresh_list = 0x00;	// 0x01 = refresh list, 0x02 cancel rename, 0x04 cancel drag, 0x08 cancel select

////////////

UINT current_dpi_tlv = USER_DEFAULT_SCREEN_DPI;
HFONT hFont_tlv = NULL;

#define _SCALE_TLV_( x )						_SCALE_( ( x ), dpi_tlv )

bool IsFilterSet( DOWNLOAD_INFO *di, unsigned int status )
{
	if ( di != NULL )
	{
		if ( status == CATEGORY_STATUS )
		{
			if ( g_category_filter != NULL && di->category == g_category_filter )
			{
				return true;
			}
		}
		else if ( ( status == STATUS_PAUSED && IS_STATUS( di->status, STATUS_PAUSED ) ) ||
				  ( status == STATUS_QUEUED && IS_STATUS( di->status, STATUS_QUEUED ) ) ||
				  ( status == STATUS_RESTART && IS_STATUS( di->status, STATUS_RESTART ) ) ||
					status == di->status )
		{
			return true;
		}
	}

	return false;
}

unsigned int TLV_GetStatusFilter()
{
	return g_status_filter;
}

void TLV_SetStatusFilter( unsigned int status )
{
	g_status_filter = status;
}

wchar_t *TLV_GetCategoryFilter()
{
	return g_category_filter;
}

void TLV_SetCategoryFilter( wchar_t *category )
{
	g_category_filter = category;
}

void TLV_ClearDrag()
{
	if ( g_is_dragging )
	{
		_ReleaseCapture();

		g_is_dragging = false;

		g_drag_rc.bottom = g_drag_rc.left = g_drag_rc.right = g_drag_rc.top = 0;

		g_draw_drag = false;
	}
}

void TLV_CancelRename( HWND hWnd )
{
	// Hide the edit textbox if it's displayed.
	if ( g_show_edit_state != 0 )
	{
		_KillTimer( hWnd, IDT_EDIT_TIMER );

		g_show_edit_state = 0;

		if ( g_hWnd_edit_box != NULL )
		{
			_SetFocus( hWnd );
		}
	}
}

int TLV_GetParentIndex( TREELISTNODE *tln, int index )
{
	if ( tln != NULL && tln->parent != NULL && index > 0 )
	{
		TREELISTNODE *tln_parent = tln->parent;

		while ( tln != tln_parent )
		{
			tln = TLV_PrevNode( tln, false, false );
			--index;
		}
	}

	return index;
}

int TLV_GetItemIndex( TREELISTNODE *tln )
{
	int index = 0;

	TREELISTNODE *node = g_tree_list;

	while ( node != NULL )
	{
		if ( node == tln )
		{
			break;
		}

		if ( g_status_filter == STATUS_NONE )
		{
			++index;
			if ( node->is_expanded )
			{
				index += node->child_count;
			}
		}
		else
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )node->data;
			if ( IsFilterSet( di, g_status_filter ) )
			{
				++index;
				if ( node->is_expanded )
				{
					index += node->child_count;
				}
			}
		}

		node = TLV_NextNode( node, false );
	}

	return index;
}

void TLV_ExpandCollapseAll( bool expand )
{
	int child_count = 0;
	int root_index = 0;

	// We need the parent of the first visible node if it's a child.
	TREELISTNODE *first_visible_node = ( g_first_visible_node != NULL && g_first_visible_node->parent != NULL ? g_first_visible_node->parent : g_first_visible_node );
	int first_visible_index = TLV_GetParentIndex( g_first_visible_node, g_first_visible_index );

	TREELISTNODE *first_selection_node = ( g_first_selection_node != NULL && g_first_selection_node->parent != NULL ? g_first_selection_node->parent : g_first_selection_node );
	TREELISTNODE *last_selection_node = ( g_last_selection_node != NULL && g_last_selection_node->parent != NULL ? g_last_selection_node->parent : g_last_selection_node );

	TREELISTNODE *base_selected_node = ( g_base_selected_node != NULL && g_base_selected_node->parent != NULL ? g_base_selected_node->parent : g_base_selected_node );
	TREELISTNODE *focused_node = ( g_focused_node != NULL && g_focused_node->parent != NULL ? g_focused_node->parent : g_focused_node );

	TREELISTNODE *tln = g_tree_list;
	while ( tln != NULL )
	{
		if ( g_status_filter != STATUS_NONE )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL && !IsFilterSet( di, g_status_filter ) )
			{
				tln = tln->next;

				continue;
			}
		}

		if ( expand )
		{
			if ( tln == first_visible_node )
			{
				g_first_visible_index += child_count;

				g_first_visible_root_index = root_index;
			}

			if ( tln == first_selection_node )
			{
				g_first_selection_index += child_count;
			}

			if ( tln == last_selection_node )
			{
				g_last_selection_index += child_count;
			}

			if ( tln == base_selected_node )
			{
				g_base_selected_index += child_count;
			}

			if ( tln == focused_node )
			{
				g_focused_index += child_count;
			}

			if ( !tln->is_expanded )
			{
				child_count += tln->child_count;

				tln->is_expanded = true;

				TLV_AddExpandedItemCount( tln->child_count );
			}
		}
		else	// Collapse
		{
			if ( tln == first_visible_node )
			{
				while ( g_root_item_count - ( first_visible_index - child_count ) < g_visible_item_count )
				{
					first_visible_node = first_visible_node->prev;
					++child_count;
				}

				g_first_visible_node = first_visible_node;
				g_first_visible_index = first_visible_index - child_count;

				g_first_visible_root_index = g_first_visible_index;
			}

			if ( tln == first_selection_node )
			{
				g_first_selection_node = tln;
				g_first_selection_index = root_index;
			}

			if ( tln == last_selection_node )
			{
				g_last_selection_node = tln;
				g_last_selection_index = root_index;
			}

			if ( tln == base_selected_node )
			{
				TREELISTNODE *selected_node = tln;
				int selected_index = root_index;

				while ( selected_node != NULL && !( selected_node->flag & TLVS_SELECTED ) )
				{
					++selected_index;
					selected_node = selected_node->next;
				}

				if ( selected_node != NULL )
				{
					g_base_selected_node = selected_node;
					g_base_selected_index = selected_index;
				}
				else
				{
					g_base_selected_node = g_first_selection_node;
					g_base_selected_index = g_first_selection_index;
				}
			}

			if ( tln == focused_node )
			{
				TREELISTNODE *selected_node = tln;
				int selected_index = root_index;

				while ( selected_node != NULL && !( selected_node->flag & TLVS_SELECTED ) )
				{
					++selected_index;
					selected_node = selected_node->next;
				}

				if ( g_focused_node != NULL )
				{
					g_focused_node->flag &= ~TLVS_FOCUSED;
				}

				if ( selected_node != NULL )
				{
					g_focused_node = selected_node;
					g_focused_index = selected_index;

					g_focused_node->flag |= TLVS_FOCUSED;
				}
				else
				{
					g_focused_node = g_first_selection_node;
					g_focused_index = g_first_selection_index;
				}
			}

			if ( tln->is_expanded )
			{
				child_count += tln->child_count;

				tln->is_expanded = false;

				TLV_AddExpandedItemCount( -( tln->child_count ) );

				// Deselect any children that are selected.
				TREELISTNODE *tln_child = tln->child;
				while ( tln_child != NULL )
				{
					if ( tln_child->flag & TLVS_SELECTED )
					{
						tln_child->flag = TLVS_NONE;

						if ( --g_selected_count <= 0 )
						{
							break;
						}
					}

					tln_child = tln_child->next;
				}
			}
		}

		++root_index;

		tln = tln->next;
	}

	g_download_history_changed = true;
}

void TLV_ExpandCollapseParent( TREELISTNODE *tln, int index, bool expand )
{
	if ( tln != NULL && index >= 0 )
	{
		if ( tln->data != NULL )
		{
			bool skip_expand_collapse = false;

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			EnterCriticalSection( &di->di_cs );
			if ( g_status_filter != STATUS_NONE )
			{
				if ( !IsFilterSet( di, g_status_filter ) || ( tln->flag & TLVS_EXPANDING_COLLAPSING ) )
				{
					skip_expand_collapse = true;
				}
				else
				{
					tln->flag |= TLVS_EXPANDING_COLLAPSING;
				}
			}
			LeaveCriticalSection( &di->di_cs );

			if ( skip_expand_collapse )
			{
				return;
			}
		}

		tln->is_expanded = expand;

		if ( expand )
		{
			TLV_AddExpandedItemCount( tln->child_count );

			// We need to update the indices of the current and last selected items as well as the selection range.

			if ( g_first_selection_index > index )
			{
				g_first_selection_index += tln->child_count;
			}

			if ( g_last_selection_index > index )
			{
				g_last_selection_index += tln->child_count;
			}

			if ( g_base_selected_index > index )
			{
				g_base_selected_index += tln->child_count;
			}

			if ( g_focused_index > index )
			{
				g_focused_index += tln->child_count;
			}
		}
		else
		{
			TLV_AddExpandedItemCount( -( tln->child_count ) );

			// Adjust the first visible node and index.
			if ( g_first_visible_index + g_visible_item_count > g_expanded_item_count )
			{
				int item_count = ( g_first_visible_index + g_visible_item_count ) - g_expanded_item_count;

				for ( ; item_count > 0 && g_first_visible_index > 0; --item_count, --g_first_visible_index )
				{
					if ( g_first_visible_node->parent == NULL )
					{
						--g_first_visible_root_index;
					}

					g_first_visible_node = TLV_PrevNode( g_first_visible_node, false );
				}
			}

			// Deselect any children that are selected.
			TREELISTNODE *tln_child = tln->child;
			while ( tln_child != NULL )
			{
				if ( tln_child->flag & TLVS_SELECTED )
				{
					tln_child->flag = TLVS_NONE;

					if ( --g_selected_count <= 0 )
					{
						break;
					}
				}

				tln_child = tln_child->next;
			}

			// We need to update the indices of the current and last selected items as well as the selection range.

			if ( g_first_selection_index > index )
			{
				// Is our first selection index a child of this node?
				if ( g_first_selection_index <= index + tln->child_count )
				{
					if ( g_selected_count > 0 )
					{
						TREELISTNODE *first_selection_node = tln;
						int first_selection_index = index;

						do
						{
							++first_selection_index;

							first_selection_node = TLV_NextNode( first_selection_node, false );
						}
						while ( first_selection_node != NULL && !( first_selection_node->flag & TLVS_SELECTED ) );

						if ( first_selection_node == NULL )
						{
							first_selection_index = -1;
						}

						g_first_selection_node = first_selection_node;
						g_first_selection_index = first_selection_index;
					}
					else
					{
						g_first_selection_node = NULL;
						g_first_selection_index = -1;
					}
				}
				else	// Offset the selection index to exclude the children.
				{
					g_first_selection_index -= tln->child_count;
				}
			}

			if ( g_last_selection_index > index )
			{
				// Is our last selection index a child of this node?
				if ( g_last_selection_index <= index + tln->child_count )
				{
					if ( g_selected_count > 0 )
					{
						TREELISTNODE *last_selection_node = tln;
						int last_selection_index = index;

						while ( last_selection_node != g_tree_list && !( last_selection_node->flag & TLVS_SELECTED ) )
						{
							--last_selection_index;

							last_selection_node = TLV_PrevNode( last_selection_node, false );
						}

						g_last_selection_node = last_selection_node;
						g_last_selection_index = last_selection_index;
					}
					else
					{
						g_last_selection_node = NULL;
						g_last_selection_index = -1;
					}
				}
				else	// Offset the selection index to exclude the children.
				{
					g_last_selection_index -= tln->child_count;
				}
			}

			//

			if ( g_base_selected_index > index )
			{
				// Is our current selected index a child of this node?
				if ( g_base_selected_index <= index + tln->child_count )
				{
					g_base_selected_node = g_first_selection_node;
					g_base_selected_index = g_first_selection_index;
				}
				else
				{
					g_base_selected_index -= tln->child_count;
				}
			}

			if ( g_focused_index > index )
			{
				// Is our last selected index a child of this node?
				if ( g_focused_index <= index + tln->child_count )
				{
					if ( g_focused_node != NULL )
					{
						g_focused_node->flag &= ~TLVS_FOCUSED;
					}

					g_focused_node = g_first_selection_node;
					g_focused_index = g_first_selection_index;

					if ( g_focused_node != NULL )
					{
						g_focused_node->flag |= TLVS_FOCUSED;
					}
				}
				else
				{
					g_focused_index -= tln->child_count;
				}
			}
		}

		if ( tln->data != NULL )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			EnterCriticalSection( &di->di_cs );
			tln->flag &= ~TLVS_EXPANDING_COLLAPSING;
			LeaveCriticalSection( &di->di_cs );
		}

		g_download_history_changed = true;
	}
}

/*int TLV_CountChildren( TREELISTNODE *tln, bool allow_collapsed )
{
	int item_count = 0;

	if ( tln != NULL )
	{
		TREELISTNODE *tln_end = tln->next;	// Direct sibling.

		tln = TLV_NextNode( tln, allow_collapsed );

		while ( tln != tln_end )
		{
			++item_count;

			tln = TLV_NextNode( tln, allow_collapsed );
		}
	}

	return item_count;
}*/

// Find the last node in the expanded list. Go through any children to find it.
TREELISTNODE *GetLastExpandedNode()
{
	// Find the last node in the expanded list. Go through any children to find it.
	TREELISTNODE *node = g_tree_list;

	do
	{
		bool is_child = false;

		while ( node != NULL )
		{
			if ( node->prev != NULL )
			{
				node = node->prev;
			}

			if ( node != NULL && node->child != NULL && node->is_expanded )
			{
				is_child = true;

				node = node->child;
			}
			else
			{
				break;
			}
		}

		if ( g_status_filter != STATUS_NONE )
		{
			TREELISTNODE *t_node = ( is_child ? node->parent : node );

			if ( t_node != NULL )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )t_node->data;
				if ( IsFilterSet( di, g_status_filter ) )
				{
					break;
				}

				node = t_node;

				if ( node == g_tree_list )
				{
					break;
				}
			}
		}
		else
		{
			break;
		}
	}
	while ( node != NULL );

	return node;
}

LONG TLV_GetTotalItemCount()
{
	return g_total_item_count;
}

LONG TLV_SetTotalItemCount( LONG total_item_count )
{
	InterlockedExchange( &g_total_item_count, total_item_count );
	return g_total_item_count;
}

LONG TLV_AddTotalItemCount( LONG value )
{
#ifdef _WIN64
	return InterlockedAdd( &g_total_item_count, value );
#else
	InterlockedExchangeAdd( &g_total_item_count, value );
	return g_total_item_count;
#endif
}

/*LONG TLV_IncTotalItemCount()
{
	return InterlockedIncrement( &g_total_item_count );
}

LONG TLV_DecTotalItemCount()
{
	return InterlockedDecrement( &g_total_item_count );
}*/

LONG TLV_GetExpandedItemCount()
{
	return g_expanded_item_count;
}

LONG TLV_SetExpandedItemCount( LONG expanded_item_count )
{
	InterlockedExchange( &g_expanded_item_count, expanded_item_count );
	return g_expanded_item_count;
}

LONG TLV_AddExpandedItemCount( LONG value )
{
#ifdef _WIN64
	return InterlockedAdd( &g_expanded_item_count, value );
#else
	InterlockedExchangeAdd( &g_expanded_item_count, value );
	return g_expanded_item_count;
#endif
}

LONG TLV_GetRootItemCount()
{
	return g_root_item_count;
}

LONG TLV_SetRootItemCount( LONG root_item_count )
{
	g_root_item_count = root_item_count;

	return g_root_item_count;
}

LONG TLV_AddRootItemCount( LONG value )
{
#ifdef _WIN64
	return InterlockedAdd( &g_root_item_count, value );
#else
	InterlockedExchangeAdd( &g_root_item_count, value );
	return g_root_item_count;
#endif
}

LONG TLV_GetParentItemNodeCount()
{
	return g_total_parent_item_nodes;
}

LONG TLV_GetSelectedCount()
{
	return g_selected_count;
}

LONG TLV_SetSelectedCount( LONG selected_count )
{
	g_selected_count = selected_count;

	return g_selected_count;
}

LONG TLV_AddSelectedCount( LONG value )
{
#ifdef _WIN64
	return InterlockedAdd( &g_selected_count, value );
#else
	InterlockedExchangeAdd( &g_selected_count, value );
	return g_selected_count;
#endif
}

TREELISTNODE *TLV_GetFocusedItem()
{
	return g_focused_node;
}

void TLV_SetFocusedItem( TREELISTNODE *tln )
{
	g_focused_node = g_base_selected_node = tln;
}

int TLV_GetFocusedIndex()
{
	return g_focused_index;
}

int TLV_SetFocusedIndex( int index )
{
	g_focused_index = g_base_selected_index = index;

	return g_focused_index;
}

TREELISTNODE *TLV_GetFirstSelectedItem()
{
	return g_first_selection_node;
}

TREELISTNODE *TLV_GetLastSelectedItem()
{
	return g_last_selection_node;
}

int TLV_GetFirstSelectedIndex()
{
	return g_first_selection_index;
}

int TLV_GetLastSelectedIndex()
{
	return g_last_selection_index;
}

int TLV_GetFirstVisibleIndex()
{
	return g_first_visible_index;
}

void TLV_SetFirstVisibleIndex( int first_visible_index )
{
	g_first_visible_index = first_visible_index;
}

int TLV_GetFirstVisibleRootIndex()
{
	return g_first_visible_root_index;
}

void TLV_SetFirstVisibleRootIndex( int first_visible_root_index )
{
	g_first_visible_root_index = first_visible_root_index;
}

TREELISTNODE *TLV_GetFirstVisibleItem()
{
	TREELISTNODE *ret_node = g_first_visible_node;

if ( g_status_filter != STATUS_NONE )
{
	while ( ret_node != NULL )
	{
		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )ret_node->data;
		if ( IsFilterSet( di, g_status_filter ) )
		{
			break;
		}

		ret_node = TLV_NextNode( ret_node, false );
	}
}
	return ret_node;
}

void TLV_SetFirstVisibleItem( TREELISTNODE *tln )
{
	g_first_visible_node = tln;
}

LONG TLV_GetVisibleItemCount()
{
	return g_visible_item_count;
}

int TLV_GetNextSelectedItem( TREELISTNODE *start_node, int start_index, TREELISTNODE **ret_node/*, bool top_level*/ )
{
	if ( ret_node == NULL )
	{
		return -1;
	}

	if ( start_node == NULL )
	{
		/*if ( top_level && g_first_selection_node != NULL && g_first_selection_node->parent != NULL )
		{
			start_index = g_first_selection_index - 1;
			start_node = g_first_selection_node;

			while ( start_node != g_first_selection_node->parent->child )
			{
				--start_index;

				start_node = TLV_PrevNode( start_node, false );
			}

			start_node = g_first_selection_node->parent;
		}
		else*/
		{
			*ret_node = g_first_selection_node;

			return g_first_selection_index;
		}
	}
	/*else if ( *start_node == g_last_selection_node )
	{
		*ret_node = g_last_selection_node;

		return g_last_selection_index;
	}
	else	// The next selected item doesn't include the one we supplied.
	{
		*ret_node = TLV_NextNode( start_node, false );
	}*/

	int selection_index = start_index;
	*ret_node = start_node;

	/*if ( top_level )
	{
		do
		{
			//selection_index += ( TLV_CountChildren( *ret_node, false ) + 1 );
			++selection_index;
			if ( ( *ret_node )->is_expanded )
			{
				selection_index += ( *ret_node )->child_count;
			}

			*ret_node = ( *ret_node )->next;
		}
		while ( *ret_node != NULL && *ret_node != g_last_selection_node && !( ( *ret_node )->flag & TLVS_SELECTED ) );
	}
	else*/
	{
		do
		{
			++selection_index;

			*ret_node = TLV_NextNode( *ret_node, false );
		}
		while ( *ret_node != NULL && *ret_node != g_last_selection_node && !( ( *ret_node )->flag & TLVS_SELECTED ) );
	}

	if ( *ret_node == NULL )
	{
		selection_index = -1;
	}
	else if ( *ret_node == g_last_selection_node )
	{
		selection_index = g_last_selection_index;
	}

	return selection_index;
}

void TLV_SelectAll( HWND hWnd, bool allow_collapsed )
{
	g_first_selection_index = -1;
	g_first_selection_node = g_tree_list;

	g_last_selection_index = -1;
	g_last_selection_node = g_tree_list;

	TREELISTNODE *tln = g_tree_list;
	if ( tln != NULL )
	{
		g_first_selection_index = 0;

		while ( tln != NULL )
		{
			tln->flag |= TLVS_SELECTED;

			g_last_selection_node = tln;

			tln = TLV_NextNode( tln, allow_collapsed );
		}
	}

	g_selected_count = ( allow_collapsed ? g_total_item_count : g_expanded_item_count );
	g_last_selection_index = g_selected_count - 1;

	_InvalidateRect( hWnd, &g_client_rc, TRUE );
}

void TLV_ResetSelectionBounds()
{
	g_first_selection_node = NULL;
	g_first_selection_index = -1;

	g_last_selection_node = NULL;
	g_last_selection_index = -1;
}

void TLV_SetSelectionBounds( int index, TREELISTNODE *tln )
{
	if ( g_first_selection_index == -1 || index < g_first_selection_index || index == -1 )
	{
		g_first_selection_node = tln;
		g_first_selection_index = index;
	}

	if ( g_last_selection_index == -1 || index > g_last_selection_index || index == -1 )
	{
		g_last_selection_node = tln;
		g_last_selection_index = index;
	}
}

TREELISTNODE *TLV_PrevNode( TREELISTNODE *node, bool allow_collapsed, bool enable_filter )
{
	TREELISTNODE *ret_node = NULL;

	while ( node != NULL )
	{
		// Make sure we're not looking at the head of the linked list.
		if ( node != node->prev )
		{
			if ( node->parent != NULL && node == node->parent->child )	// Node is the first child of its parent.
			{
				ret_node = node->parent;
			}
			else
			{
				// Either get the last node of the current node's children, or get it's previous sibling.
				while ( node != NULL && ( node->child != NULL || node->prev != NULL ) )
				{
					if ( node->prev != NULL )
					{
						node = node->prev;
					}

					if ( node->child != NULL && ( node->is_expanded || allow_collapsed ) )
					{
						node = node->child;
					}
					else
					{
						break;
					}
				}

				ret_node = node;
			}
		}

		if ( enable_filter && g_status_filter != STATUS_NONE && ret_node != NULL  )
		{
			// If it's a child node, then test the parent.
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )( ret_node->parent == NULL ? ret_node->data : ret_node->parent->data );
			if ( IsFilterSet( di, g_status_filter ) )
			{
				break;
			}

			if ( ret_node == g_tree_list )
			{
				break;
			}

			node = ret_node;
		}
		else
		{
			break;
		}
	}

	return ret_node;
}

TREELISTNODE *TLV_NextNode( TREELISTNODE *node, bool allow_collapsed, bool enable_filter )
{
	TREELISTNODE *ret_node = NULL;

	while ( node != NULL )
	{
		if ( node != NULL )
		{
			// If the node is a parent with children.
			if ( node->child != NULL && ( node->is_expanded || allow_collapsed ) )
			{
				ret_node = node->child;
			}
			else if ( node->next == NULL && node->parent != NULL )	// Node is a child with no more siblings. Go to its parent's sibling.
			{
				do
				{
					node = node->parent;
				}
				while ( node->next == NULL && node->parent != NULL );

				ret_node = node->next;
			}
			else
			{
				ret_node = node->next;
			}
		}

		if ( enable_filter && g_status_filter != STATUS_NONE && ret_node != NULL )
		{
			// If it's a child node, then test the parent.
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )( ret_node->parent == NULL ? ret_node->data : ret_node->parent->data );
			if ( IsFilterSet( di, g_status_filter ) )
			{
				break;
			}

			node = ret_node;
		}
		else
		{
			break;
		}
	}

	return ret_node;
}

void TLV_AddNode( TREELISTNODE **head, TREELISTNODE *node, int position, bool is_child )
{
	if ( node == NULL )
	{
		return;
	}

	if ( !is_child )
	{
		++g_total_parent_item_nodes;
	}

	if ( *head == NULL )
	{
		*head = node;
		return;
	}

	if ( position < 0 )	// Insert node as the new tail.
	{
		node->next = NULL;

		if ( ( *head )->prev != NULL )	// Head has a tail.
		{
			node->prev = ( *head )->prev;
			( *head )->prev->next = node;
		}
		else							// Head has no tail.
		{
			node->prev = *head;
			( *head )->next = node;
		}
		
		( *head )->prev = node;
	}
	else if ( position == 0 )	// Insert node as the new head.
	{
		node->next = *head;
		node->prev = ( *head )->prev;
		( *head )->prev = node;

		*head = node;	// Set the new head.
	}
	else
	{
		int count = 0;
		TREELISTNODE *last_node = *head;
		TREELISTNODE *current_node = ( *head )->next;
		while ( current_node != NULL )
		{
			if ( ++count == position )
			{
				node->next = current_node;
				node->prev = last_node;
				last_node->next = node;
				return;
			}

			last_node = current_node;
			current_node = current_node->next;
		}

		// The position is at the end of the list. Add node as the new tail.
		if ( current_node == NULL && ++count == position )
		{
			node->next = current_node;
			node->prev = last_node;
			last_node->next = node;

			( *head )->prev = node;
		}
	}
}

void TLV_RemoveNode( TREELISTNODE **head, TREELISTNODE *node, bool is_child )
{
	if ( *head != NULL && node != NULL )
	{
		if ( !is_child )
		{
			--g_total_parent_item_nodes;
		}

		if ( node == *head ) // Node is the head.
		{
			if ( node->next != NULL )	// See if we can make a new head.
			{
				if ( node->next != node->prev )	// Make sure the new tail's previous value isn't itself.
				{
					node->next->prev = node->prev;	// Set the new tail.
				}
				else
				{
					node->next->prev = NULL;
				}

				*head = node->next;
			}
			else	// No head exists now.
			{
				*head = NULL;
			}
		}
		else if ( node->next == NULL )	// Node is a tail.
		{
			if ( node->prev != NULL )	// This should always be the case so long as node != head.
			{
				if ( node->prev != *head )	// Make sure the node's previous value is not the head.
				{
					if ( ( *head )->prev == node )	// Make sure the head list actually contains the node we're removing.
					{
						( *head )->prev = node->prev;	// Set the new tail.
					}

					node->prev->next = NULL;
				}
				else	// All that exists now is the head.
				{
					( *head )->next = NULL;
					( *head )->prev = NULL;
				}
			}
		}
		else if ( node->next != NULL && node->prev != NULL )	// Node is between two other nodes.
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}

		node->next = NULL;
		node->prev = NULL;
	}
}

int TLV_FreeTree( TREELISTNODE *tln )
{
	int node_count = 0;

	while ( tln != NULL )
	{
		TREELISTNODE *del_node;

		if ( tln->child != NULL )
		{
			del_node = tln->child;

			// Insert the child list between the parent and its sibling.
			if ( del_node->prev != NULL )
			{
				del_node->prev->next = tln->next;
			}
			else
			{
				del_node->next = tln->next;
			}

			tln->next = del_node;
		}

		del_node = tln;
		tln = tln->next;

		GlobalFree( del_node );

		++node_count;
	}

	return node_count;
}

void TLV_ClearSelected( bool ctrl_down, bool shift_down )
{
	int selected_count = g_selected_count;

	TREELISTNODE *node = g_first_selection_node;
	while ( node != NULL )
	{
		if ( node->flag & TLVS_SELECTED )
		{
			--selected_count;
		}

		if ( ctrl_down || shift_down )
		{
			node->flag &= ~TLVS_FOCUSED;
		}
		else
		{
			node->flag = TLVS_NONE;
		}

		if ( selected_count <= 0 )
		{
			break;
		}

		if ( node != g_last_selection_node )
		{
			node = TLV_NextNode( node, true );
		}
		else
		{
			break;
		}
	}
}

void TLV_CleanupSort()
{
	TREELISTNODE *tli_node = g_tree_list;

	int first_visible_root_index = 0;

	if ( tli_node != NULL )
	{
		// Iterate to the first visible node from the closest end of our list.
		if ( g_first_visible_index <= ( g_expanded_item_count / 2 ) )
		{
			for ( int i = 1; i <= g_first_visible_index; ++i )
			{
				tli_node = TLV_NextNode( tli_node, false );

				if ( tli_node != NULL )
				{
					if ( tli_node->parent == NULL )
					{
						++first_visible_root_index;
					}
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			tli_node = GetLastExpandedNode();

			for ( int i = g_expanded_item_count - 1; i > g_first_visible_index && tli_node != g_tree_list; --i )
			{
				if ( tli_node->parent == NULL )
				{
					++first_visible_root_index;
				}

				tli_node = TLV_PrevNode( tli_node, false );
			}

			first_visible_root_index = ( g_root_item_count - 1 ) - first_visible_root_index;
		}
	}

	g_first_visible_node = tli_node;

	g_first_visible_root_index = first_visible_root_index;

	g_first_selection_node = g_tree_list;
	g_first_selection_index = -1;

	g_last_selection_node = NULL;
	g_last_selection_index = -1;
}

int merge_compare( void *a, void *b, void *sort )
{
	SORT_INFO *si = ( SORT_INFO * )sort;

	DOWNLOAD_INFO *di1 = ( DOWNLOAD_INFO * )( ( si->direction == 1 ) ? a : b );
	DOWNLOAD_INFO *di2 = ( DOWNLOAD_INFO * )( ( si->direction == 1 ) ? b : a );

	switch ( si->column )
	{
		case COLUMN_CATEGORY:				{ return _wcsicmp_s( di1->shared_info->category, di2->shared_info->category ); } break;
		case COLUMN_COMMENTS:				{ return _wcsicmp_s( di1->comments, di2->comments ); } break;
		case COLUMN_DOWNLOAD_DIRECTORY:		{ return _wcsicmp_s( di1->shared_info->file_path, di2->shared_info->file_path ); } break;
		case COLUMN_FILE_TYPE:				{ return _wcsicmp_s( di1->shared_info->file_path + di1->shared_info->file_extension_offset, di2->shared_info->file_path + di2->shared_info->file_extension_offset ); } break;
		case COLUMN_FILENAME:				{ return _wcsicmp_s( di1->shared_info->file_path + di1->shared_info->filename_offset, di2->shared_info->file_path + di2->shared_info->filename_offset ); } break;
		case COLUMN_URL:					{ return _wcsicmp_s( di1->url, di2->url ); } break;

		case COLUMN_DOWNLOAD_SPEED:			{ return ( di1->shared_info->speed > di2->shared_info->speed ); } break;
		case COLUMN_DOWNLOAD_SPEED_LIMIT:	{ return ( di1->download_speed_limit > di2->download_speed_limit ); } break;
		case COLUMN_DOWNLOADED:				{ return ( di1->shared_info->downloaded > di2->shared_info->downloaded ); } break;
		case COLUMN_FILE_SIZE:				{ return ( di1->shared_info->file_size > di2->shared_info->file_size ); } break;
		case COLUMN_DATE_AND_TIME_ADDED:	{ return ( di1->shared_info->add_time.QuadPart > di2->shared_info->add_time.QuadPart ); } break;
		case COLUMN_TIME_ELAPSED:			{ return ( di1->shared_info->time_elapsed > di2->shared_info->time_elapsed ); } break;
		case COLUMN_TIME_REMAINING:			{ return ( di1->shared_info->time_remaining > di2->shared_info->time_remaining ); } break;

		case COLUMN_PROGRESS:
		{
			if ( di1->status == di2->status )
			{
				if ( di1->status == STATUS_FAILED )
				{
					char di1_type = 0, di2_type = 0;

					if ( di1->url != NULL )
					{
						if ( di1->url[ 0 ] == L'h' || di1->url[ 0 ] == L'H' )
						{
							di1_type = 2;
						}
						else if ( di1->url[ 0 ] == L'f' || di1->url[ 0 ] == L'F' )
						{
							di1_type = 1;
						}
						else if ( di1->url[ 0 ] == L's' || di1->url[ 0 ] == L'S' )
						{
							di1_type = 3;
						}
					}

					if ( di2->url != NULL )
					{
						if ( di2->url[ 0 ] == L'h' || di2->url[ 0 ] == L'H' )
						{
							di2_type = 2;
						}
						else if ( di2->url[ 0 ] == L'f' || di2->url[ 0 ] == L'F' )
						{
							di2_type = 1;
						}
						else if ( di2->url[ 0 ] == L's' || di2->url[ 0 ] == L'S' )
						{
							di2_type = 3;
						}
					}

					if ( di1_type == di2_type )
					{
						if ( di1->code != di2->code )
						{
							return ( di1->code > di2->code );
						}
					}
					else
					{
						return ( di1_type > di2_type );
					}
				}

				if ( di1->last_downloaded != 0 && di2->last_downloaded == 0 )
				{
					return 1;
				}
				else if ( di1->last_downloaded == 0 && di2->last_downloaded != 0 )
				{
					return -1;
				}
				else if ( di1->last_downloaded == 0 && di2->last_downloaded == 0 )
				{
					return 0;
				}
				else
				{
					if ( di1->file_size != 0 && di2->file_size == 0 )
					{
						return 1;
					}
					else if ( di1->file_size == 0 && di2->file_size != 0 )
					{
						return -1;
					}
					else if ( di1->file_size == 0 && di2->file_size == 0 )
					{
						return 0;
					}

					int i_percentage1;
					int i_percentage2;
#ifdef _WIN64
					i_percentage1 = ( int )( 1000.0 * ( ( double )di1->last_downloaded / ( double )di1->file_size ) );
					i_percentage2 = ( int )( 1000.0 * ( ( double )di2->last_downloaded / ( double )di2->file_size ) );
#else
					// Multiply the floating point division by 1000%.
					// This leaves us with an integer in which the last digit will represent the decimal value.
					double f_percentage1 = 1000.0 * ( ( double )di1->last_downloaded / ( double )di1->file_size );
					__asm
					{
						fld f_percentage1;		//; Load the floating point value onto the FPU stack.
						fistp i_percentage1;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
					}

					// Multiply the floating point division by 1000%.
					// This leaves us with an integer in which the last digit will represent the decimal value.
					double f_percentage2 = 1000.0 * ( ( double )di2->last_downloaded / ( double )di2->file_size );
					__asm
					{
						fld f_percentage2;		//; Load the floating point value onto the FPU stack.
						fistp i_percentage2;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
					}
#endif
					return ( i_percentage1 > i_percentage2 );
				}

				//return ( ( di1->file_size - di1->downloaded ) > ( di2->file_size - di2->downloaded ) );
			}
			else
			{
				return ( di1->status > di2->status );
			}
		}
		break;

		case COLUMN_SSL_TLS_VERSION:
		{
			if ( di1->ssl_version == -1 )
			{
				return -1;
			}
			else if ( di2->ssl_version == -1 )
			{
				return 1;
			}
			else
			{
				return ( di1->ssl_version > di2->ssl_version );
			}
		}
		break;

		case COLUMN_ACTIVE_PARTS:
		{
			if ( di1->parts > di2->parts )
			{
				return 1;
			}
			else if ( di1->parts < di2->parts )
			{
				return -1;
			}
			else
			{
				return ( di1->active_parts > di2->active_parts );
			}
		}
		break;

		default:
		{
			return 0;
		}
		break;
	}
}

void TLV_MergeSort( TREELISTNODE **head, int length, int ( *compare )( void *a, void *b, void *s ), void *sort_info )
{
	if ( head != NULL && *head != NULL )
	{
		TREELISTNODE *start1 = NULL, *end1 = NULL;
		TREELISTNODE *start2 = NULL, *end2 = NULL;
		TREELISTNODE *prevend = NULL;

		for ( int sub_length = 1; sub_length < length; sub_length <<= 1 )
		{
			start1 = *head;

			while ( start1 != NULL )
			{
				// If this is first iteration
				bool isFirstIter = ( start1 == *head ? true : false );

				// First part for merging
				int counter = sub_length;
				end1 = start1;
				while ( --counter > 0 && end1->next != NULL )
				{
					end1 = end1->next;
				}

				// Second part for merging
				start2 = end1->next;
				if ( start2 == NULL )
				{
					break;
				}

				counter = sub_length;
				end2 = start2;
				while ( --counter > 0 && end2->next != NULL )
				{
					end2 = end2->next;
				}

				// To store for next iteration.
				TREELISTNODE *tmp = end2->next;

				// Merging two parts.
				/////////////////////

				// Make sure that first node of second list is higher.
				TREELISTNODE *tmp2 = NULL;

				if ( compare( start1->data, start2->data, sort_info ) > 0 )
				{
					tmp2 = start2;
					start2 = start1;
					start1 = tmp2;

					tmp2 = end2;
					end2 = end1;
					end1 = tmp2;
				}

				// Merging remaining nodes.
				TREELISTNODE *astart = start1, *aend = end1;
				TREELISTNODE *bstart = start2;
				TREELISTNODE *bendnext = end2->next;

				while ( astart != aend && bstart != bendnext )
				{
					if ( compare( astart->next->data, bstart->data, sort_info ) > 0 )
					{
						tmp2 = bstart->next;
						bstart->next = astart->next;
						astart->next->prev = bstart;
						astart->next = bstart;
						bstart->prev = astart;
						bstart = tmp2;
					}

					astart = astart->next;
				}

				if ( astart == aend )
				{
					astart->next = bstart;

					if ( bstart != NULL )
					{
						bstart->prev = astart;
					}
				}
				else
				{
					end2 = end1;
				}

				/////////////////////
				/////////////////////

				// Update head for first iteration, else append after previous list.
				if ( isFirstIter )
				{
					*head = start1;
				}
				else
				{
					if ( prevend != NULL )
					{
						prevend->next = start1;

						if ( start1 != NULL )
						{
							start1->prev = prevend;
						}
					}
				}

				prevend = end2;
				start1 = tmp;
			}

			if ( prevend != NULL )
			{
				prevend->next = start1;

				if ( start1 != NULL )
				{
					start1->prev = prevend;
				}
			}
		}

		if ( *head != NULL )
		{
			( *head )->prev = end2;
		}
	}
}

int Scroll( SCROLLINFO *si, unsigned char type, int scroll_amount )
{
	int offset = 0;

	if ( type == SCROLL_TYPE_UP || type == SCROLL_TYPE_LEFT )
	{
		if ( si != NULL && si->nPos > 0 )
		{
			if ( si->nPos > scroll_amount )
			{
				offset = scroll_amount;
			}
			else
			{
				offset = si->nPos;
			}

			si->nPos -= offset;

			if ( type == SCROLL_TYPE_UP )
			{
				for ( int i = 0; i < offset && g_first_visible_node != g_tree_list; ++i )
				{
					if ( g_first_visible_node != NULL && g_first_visible_node->parent == NULL )
					{
						--g_first_visible_root_index;
					}

					g_first_visible_node = TLV_PrevNode( g_first_visible_node, false );
				}

				g_first_visible_index = si->nPos;
			}
		}
	}
	else if ( type == SCROLL_TYPE_DOWN || type == SCROLL_TYPE_RIGHT )
	{
		if ( si != NULL && ( si->nPos + si->nPage ) <= ( unsigned int )si->nMax )
		{
			if ( ( si->nPos + si->nPage + scroll_amount ) > ( unsigned int )si->nMax )
			{
				offset = ( si->nMax - ( si->nPos + si->nPage ) ) + 1;
			}
			else
			{
				offset = scroll_amount;
			}

			si->nPos += offset;

			if ( type == SCROLL_TYPE_DOWN )
			{
				TREELISTNODE *first_visible_node = TLV_GetFirstVisibleItem();

				for ( int i = 0; i < offset && first_visible_node != NULL; ++i )
				{
					first_visible_node = TLV_NextNode( first_visible_node, false );

					if ( first_visible_node != NULL  )
					{
						g_first_visible_node = first_visible_node;

						if ( g_first_visible_node->parent == NULL )
						{
							++g_first_visible_root_index;
						}
					}
				}

				g_first_visible_index = si->nPos;
			}
		}
	}

	return offset;
}

void TLV_Scroll( HWND hWnd, char scroll_type, WORD direction )	// scroll_type: 0 = vertical, 1 = horizontal
{
	SCROLLINFO si;
	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;

	int offset = 0;

	if ( scroll_type == 0 )	// Vertical
	{
		_GetScrollInfo( hWnd, SB_VERT, &si );

		switch ( direction )
		{
			case SB_LINEUP:
			{
				offset = Scroll( &si, SCROLL_TYPE_UP, 1 );
			}
			break;

			case SB_LINEDOWN:
			{
				offset = Scroll( &si, SCROLL_TYPE_DOWN, 1 );
			}
			break;

			case SB_PAGEUP:
			{
				offset = Scroll( &si, SCROLL_TYPE_UP, si.nPage );
			}
			break;

			case SB_PAGEDOWN:
			{
				offset = Scroll( &si, SCROLL_TYPE_DOWN, si.nPage );
			}
			break;

			case SB_THUMBTRACK:
			{
				if ( si.nPos > si.nTrackPos )
				{
					offset = Scroll( &si, SCROLL_TYPE_UP, si.nPos - si.nTrackPos );
				}
				else if ( si.nPos < si.nTrackPos )
				{
					offset = Scroll( &si, SCROLL_TYPE_DOWN, si.nTrackPos - si.nPos );
				}
			}
			break;

			case SB_TOP:
			{
				offset = Scroll( &si, SCROLL_TYPE_UP, si.nMax );
			}
			break;

			case SB_BOTTOM:
			{
				offset = Scroll( &si, SCROLL_TYPE_DOWN, si.nMax );
			}
			break;
		}

		if ( offset != 0 )
		{
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

			_InvalidateRect( hWnd, &g_client_rc, TRUE );
		}
	}
	else if ( scroll_type == 1 )	// Horizontal
	{
		_GetScrollInfo( hWnd, SB_HORZ, &si );

		switch ( direction )
		{
			case SB_LINELEFT:
			{
				offset = Scroll( &si, SCROLL_TYPE_LEFT, hs_step );
			}
			break;

			case SB_LINERIGHT:
			{
				offset = Scroll( &si, SCROLL_TYPE_RIGHT, hs_step );
			}
			break;

			case SB_PAGELEFT:
			{
				offset = Scroll( &si, SCROLL_TYPE_LEFT, si.nPage );
			}
			break;

			case SB_PAGERIGHT:
			{
				offset = Scroll( &si, SCROLL_TYPE_RIGHT, si.nPage );
			}
			break;

			case SB_THUMBTRACK:
			{
				if ( si.nPos > si.nTrackPos )
				{
					offset = Scroll( &si, SCROLL_TYPE_LEFT, si.nPos - si.nTrackPos );
				}
				else if ( si.nPos < si.nTrackPos )
				{
					offset = Scroll( &si, SCROLL_TYPE_RIGHT, si.nTrackPos - si.nPos );
				}
			}
			break;

			case SB_LEFT:
			{
				offset = Scroll( &si, SCROLL_TYPE_LEFT, si.nMax );
			}
			break;

			case SB_RIGHT:
			{
				offset = Scroll( &si, SCROLL_TYPE_RIGHT, si.nMax );
			}
			break;
		}

		if ( offset != 0 )
		{
			HDWP hdwp = _BeginDeferWindowPos( 1 );
			_DeferWindowPos( hdwp, g_hWnd_tlv_header, HWND_TOP, -si.nPos, 0, g_client_rc.right + si.nPos, g_header_height, 0 );
			_EndDeferWindowPos( hdwp );

			_SetScrollInfo( hWnd, SB_HORZ, &si, TRUE );

			_InvalidateRect( hWnd, &g_client_rc, TRUE );
		}
	}
}

VOID CALLBACK ScrollTimerProc( HWND hWnd, UINT /*msg*/, UINT /*idTimer*/, DWORD /*dwTime*/ )
{
	// Vertical

	SCROLLINFO si;
	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;
	_GetScrollInfo( hWnd, SB_VERT, &si );

	int offset = 0;

	if ( g_v_scroll_direction == 1 )	// Down
	{
		offset = Scroll( &si, SCROLL_TYPE_DOWN, g_v_scroll_line_amount );

		g_drag_pos.y -= ( offset * g_row_height );
	}
	else if ( g_v_scroll_direction == 2 )	// Up
	{
		offset = Scroll( &si, SCROLL_TYPE_UP, g_v_scroll_line_amount );

		g_drag_pos.y += ( offset * g_row_height );
	}

	if ( offset != 0 )
	{
		_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
	}

	// Horizontal

	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_ALL;
	_GetScrollInfo( hWnd, SB_HORZ, &si );

	offset = 0;

	if ( g_h_scroll_direction == 1 )	// Left
	{
		offset = Scroll( &si, SCROLL_TYPE_LEFT, g_h_scroll_line_amount );

		g_drag_pos.x += offset;
	}
	else if ( g_h_scroll_direction == 2 )	// Right
	{
		offset = Scroll( &si, SCROLL_TYPE_RIGHT, g_h_scroll_line_amount );

		g_drag_pos.x -= offset;
	}

	if ( offset != 0 )
	{
		HDWP hdwp = _BeginDeferWindowPos( 1 );
		_DeferWindowPos( hdwp, g_hWnd_tlv_header, HWND_TOP, -si.nPos, 0, g_client_rc.right + si.nPos, g_header_height, 0 );
		_EndDeferWindowPos( hdwp );

		_SetScrollInfo( hWnd, SB_HORZ, &si, TRUE );
	}

	// Allow the lasso selection if we're not in the edit mode, or we've dragged from outside of the item list.
	if ( !g_in_list_edit_mode || g_drag_start_index == -1 || g_drag_pos.x > g_header_width )
	{
		HandleMouseMovement( hWnd );
	}
	else
	{
		HandleMouseDrag( hWnd );
	}
}

void HandleWindowChange( HWND hWnd, bool scroll_to_end = false, bool adjust_column = true )
{
	if ( g_skip_window_change )
	{
		return;
	}

	_GetClientRect( hWnd, &g_client_rc );

	g_client_rc.top = g_header_height;	// Offset the top of our list area to the bottom of the header control.

	g_visible_rows = ( ( g_client_rc.bottom - g_client_rc.top ) / g_row_height ) + 1;

	g_visible_item_count = min( ( g_visible_rows - 1 ), g_expanded_item_count );

	int delta;
	SCROLLINFO si;

	// Vertical Scroll

	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_RANGE | SIF_POS;
	_GetScrollInfo( hWnd, SB_VERT, &si );

	if ( g_expanded_item_count < g_visible_rows )
	{
		g_first_visible_node = g_tree_list;
		g_first_visible_index = 0;
		g_first_visible_root_index = 0;
	}
	else if ( g_expanded_item_count >= si.nMax )
	{
		// We need to determine if the window is increasing in height and if so, then we need to move the first visible item up.
		delta = ( g_visible_rows - 1 ) - ( g_expanded_item_count - si.nPos );
		if ( delta > 0 )
		{
			Scroll( &si, SCROLL_TYPE_UP, delta );
		}
	}

	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = g_expanded_item_count;
	si.nPage = g_visible_rows;
	si.nPos = g_first_visible_index;

	if ( scroll_to_end )
	{
		if ( g_tree_list != NULL )
		{
			TLV_ClearDrag();
			TLV_ClearSelected( false, false );

			TREELISTNODE *focused_node;
			int focused_index;

			if ( g_tree_list->prev != NULL )
			{
				focused_node = g_tree_list->prev;
			}
			else
			{
				focused_node = g_tree_list;
			}

			focused_index = g_expanded_item_count - 1;

			if ( focused_node != NULL )
			{
				if ( focused_node->is_expanded && focused_node->child != NULL )
				{
					if ( focused_node->child->prev != NULL )
					{
						focused_node = focused_node->child->prev;
					}
					else
					{
						focused_node = focused_node->child;
					}
				}

				/*if ( focused_node->parent != NULL )
				{
					focused_index = TLV_GetParentIndex( focused_node, focused_index );
					focused_node = focused_node->parent;
				}*/

				focused_node->flag |= ( TLVS_SELECTED | TLVS_FOCUSED );

				g_selected_count = 1;
			}
			else
			{
				focused_index = -1;
				g_selected_count = 0;
			}

			g_focused_node = g_first_selection_node = g_last_selection_node = g_base_selected_node = focused_node;
			g_focused_index = g_first_selection_index = g_last_selection_index = g_base_selected_index = focused_index;

			Scroll( &si, SCROLL_TYPE_DOWN, si.nMax );
		}

		//Scroll( &si, SCROLL_TYPE_DOWN, g_expanded_item_count );
	}

	_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

	///////////////

	// Horizontal Scroll (the header control will be positioned with si.nPos)

	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	_GetScrollInfo( hWnd, SB_HORZ, &si );

	// If we're adjusting the width of the last column header, then set the nPos.
	if ( adjust_column )
	{
		delta = si.nMax - g_header_width;
		if ( delta > 0 )
		{
			Scroll( &si, SCROLL_TYPE_LEFT, delta );
		}
	}

	// If we're adjusting the width of the window, then set the nPos.
	delta = g_client_rc.right - si.nPage;
	if ( si.nPos > 0 && delta > 0 )
	{
		if ( delta >= si.nPos )
		{
			si.nPos = 0;
		}
		else if ( ( si.nPage + si.nPos ) > ( unsigned int )si.nMax )
		{
			si.nPos -= delta;
		}
	}

	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = g_header_width;
	si.nPage = g_client_rc.right;

	_SetScrollInfo( hWnd, SB_HORZ, &si, TRUE );

	///////////////

	HDWP hdwp = _BeginDeferWindowPos( 1 );
	_DeferWindowPos( hdwp, g_hWnd_tlv_header, HWND_TOP, -si.nPos, 0, g_client_rc.right + si.nPos, g_header_height, 0 );
	_EndDeferWindowPos( hdwp );
}

void HandleMouseMovement( HWND hWnd )
{
	if ( g_is_dragging )
	{
		POINT cur_pos;
		_GetCursorPos( &cur_pos );
		_ScreenToClient( hWnd, &cur_pos );

		// Update our drag rectangle.
		if ( cur_pos.x > g_drag_pos.x )
		{
			g_drag_rc.left = g_drag_pos.x;
			g_drag_rc.right = cur_pos.x;
		}
		else// if ( cur_pos.x < g_drag_pos.x )
		{
			g_drag_rc.left = cur_pos.x;
			g_drag_rc.right = g_drag_pos.x;
		}

		if ( cur_pos.y > g_drag_pos.y )
		{
			g_drag_rc.top = g_drag_pos.y;
			g_drag_rc.bottom = cur_pos.y;
		}
		else// if ( cur_pos.y < g_drag_pos.y )
		{
			g_drag_rc.top = cur_pos.y;
			g_drag_rc.bottom = g_drag_pos.y;
		}

		// Hide the edit textbox if it's displayed and we've clicked outside the Filename column.
		if ( g_show_edit_state != 0 )
		{
			_KillTimer( hWnd, IDT_EDIT_TIMER );

			g_show_edit_state = 0;
		}

		TREELISTNODE *node;

		int pick_index = ( cur_pos.y - g_client_rc.top ) / g_row_height;
		if ( pick_index < 0 )
		{
			pick_index = 0;
		}
		pick_index += g_first_visible_index;

		// From the first visible node, get the node that was clicked.
		TREELISTNODE *tli_node = TLV_GetFirstVisibleItem();
		for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
		{
			tli_node = TLV_NextNode( tli_node, false );
		}

		// See if any modifier key is pressed.
		bool ctrl_down = ( _GetKeyState( VK_CONTROL ) & 0x8000 ) ? true : false;
		bool shift_down = ( _GetKeyState( VK_SHIFT ) & 0x8000 ) && !ctrl_down ? true : false;

		if ( ctrl_down || shift_down )
		{
			if ( !g_mod_key_active )
			{
				g_mod_key_active = true;

				g_mod_first_selection_node = g_first_selection_node;
				g_mod_first_selection_index = g_first_selection_index;

				g_mod_last_selection_node = g_last_selection_node;
				g_mod_last_selection_index = g_last_selection_index;
			}
		}
		else
		{
			g_mod_key_active = false;
		}

		// We've started dragging from a row beyond the width of the header (passed the last column).
		if ( g_drag_pos.x > g_header_width )
		{
			// We're still outside the width of the header.
			if ( cur_pos.x > g_header_width )
			{
				// If we've set a focus index (the node may not be valid), then we need to process all items within the range of the base selected index and focused index.
				if ( g_focused_index != -1 )
				{
					int index = -1;

					// We've started our drag from an area where the row has no valid item and there's no column (bottom right).
					if ( g_base_selected_node == NULL )
					{
						// We've dragged back into an area where there's no column (but the row has a valid item).
						// Get the last item's node and index.
						if ( g_focused_index < g_expanded_item_count )
						{
							node = GetLastExpandedNode();
							index = g_expanded_item_count - 1;
						}
						else	// We've dragged back into an area where the row has no valid item and there's no column (bottom right).
						{
							node = NULL;
						}
					}
					else	// We've started our drag in an area where there's no column (but the row has a valid item).
					{
						//g_base_selected_node->flag &= ~TLVS_FOCUSED;

						node = g_base_selected_node;
						index = g_base_selected_index;
					}

					// We have a node after dragging outside the width of the header.
					if ( node != NULL )
					{
						TREELISTNODE *start_node = NULL;
						TREELISTNODE *end_node = NULL;

						if ( g_focused_index >= index )
						{
							start_node = node;
							end_node = g_focused_node;
						}
						else// if ( g_focused_index < index )
						{
							start_node = g_focused_node;
							end_node = node;
						}

						// Go through each item between the focused item and the base selected item and remove or invert their selection.
						for ( ;; )
						{
							if ( start_node != NULL )
							{
								if ( ctrl_down )
								{
									if ( start_node->flag & TLVS_SELECTED )
									{
										--g_selected_count;
									}
									else
									{
										++g_selected_count;
									}

									start_node->flag ^= TLVS_SELECTED;
									start_node->flag &= ~TLVS_FOCUSED;
								}
								else
								{
									--g_selected_count;

									start_node->flag = TLVS_NONE;
								}

								if ( start_node != end_node )
								{
									start_node = TLV_NextNode( start_node, false );

									continue;
								}
							}

							break;
						}
					}

					g_drag_start_index = -1;

					g_focused_node = NULL;
					g_focused_index = -1;
				}

				goto SCROLL_WINDOW;
			}

			// We're now dragging within the area of the header.
			if ( g_focused_index == -1 )
			{
				// If the focus index hasn't been set, then the base selected node should have been in WM_LBUTTONDOWN.
				if ( g_base_selected_node != NULL )
				{
					// Invert the selection.
					if ( ctrl_down )
					{
						if ( g_base_selected_node->flag & TLVS_SELECTED )
						{
							--g_selected_count;
						}
						else
						{
							++g_selected_count;
						}

						g_base_selected_node->flag ^= TLVS_SELECTED;
					}
					else	// Add to the selection.
					{
						if ( !( g_base_selected_node->flag & TLVS_SELECTED ) )
						{
							++g_selected_count;
						}

						g_base_selected_node->flag |= TLVS_SELECTED;	// Selected
					}

					g_base_selected_node->flag |= TLVS_FOCUSED;	// Focused
				}

				// We want the drag start index to be a valid index.
				if ( g_base_selected_index < g_expanded_item_count )
				{
					g_drag_start_index = g_base_selected_index;
				}

				g_focused_node = g_base_selected_node;	// This may not be valid, but that's okay.
				g_focused_index = g_base_selected_index;	// We want the focused index to be a valid index.
			}
		}

		// Make sure we're working with valid indices.
		int drag_start_index = ( g_drag_start_index == -1 ? ( g_expanded_item_count - 1 ) : g_drag_start_index );
		int focused_index ( g_focused_index == -1 ? pick_index : g_focused_index );

		// We've moved over another index that isn't the currently focused index.
		if ( pick_index != g_focused_index )
		{
			if ( pick_index < focused_index )		// We're moving up.
			{
				// Select the last item if we're starting from a negative index.
				if ( focused_index >= g_expanded_item_count &&
					 pick_index < g_expanded_item_count )
				{
					g_focused_node = GetLastExpandedNode();
					focused_index = g_expanded_item_count - 1;
				}

				node = g_focused_node;

				if ( node != NULL )
				{
					// We started our selection and dragged down, then we dragged up.
					// The items that were selected while dragging down will need to be cleared when dragging up.
					// Go through each item between the drag_start_index, focused item and the picked item and remove or invert their selection.
					while ( focused_index > drag_start_index && focused_index > pick_index )
					{
						if ( node != NULL )
						{
							if ( ctrl_down )
							{
								if ( node->flag & TLVS_SELECTED )
								{
									--g_selected_count;
								}
								else
								{
									++g_selected_count;
								}

								node->flag ^= TLVS_SELECTED;
							}
							else
							{
								--g_selected_count;

								node->flag = TLVS_NONE;
							}

							node = TLV_PrevNode( node, false );
						}

						--focused_index;
					}
				}

				node = TLV_PrevNode( node, false );

				if ( node != NULL )
				{
					// We've started our selection and dragged up.
					// Go through each item between the focused item and the picked item and add or invert their selection.
					while ( focused_index > pick_index )
					{
						if ( node != NULL )
						{
							if ( ctrl_down )
							{
								if ( node->flag & TLVS_SELECTED )
								{
									--g_selected_count;
								}
								else
								{
									++g_selected_count;
								}

								node->flag ^= TLVS_SELECTED;
							}
							else
							{
								if ( !( node->flag & TLVS_SELECTED ) )
								{
									++g_selected_count;
								}

								node->flag |= TLVS_SELECTED;
							}

							node = TLV_PrevNode( node, false );
						}

						--focused_index;
					}
				}

				// Select the last node if we're moving from a negative index.
				if ( g_drag_start_index == -1 &&
					 pick_index < g_expanded_item_count &&
					 g_focused_index >= g_expanded_item_count )
				{
					g_focused_node = GetLastExpandedNode();

					if ( g_focused_node != NULL )
					{
						if ( ctrl_down )
						{
							if ( g_focused_node->flag & TLVS_SELECTED )
							{
								--g_selected_count;
							}
							else
							{
								++g_selected_count;
							}

							g_focused_node->flag ^= TLVS_SELECTED;
						}
						else
						{
							if ( !( g_focused_node->flag & TLVS_SELECTED ) )
							{
								++g_selected_count;
							}

							g_focused_node->flag |= TLVS_SELECTED;
						}
					}
				}
			}
			else if ( pick_index > focused_index )		// We're moving down.
			{
				node = g_focused_node;

				if ( node != NULL )
				{
					// We started our selection and dragged up, then we dragged down.
					// The items that were selected while dragging up will need to be cleared when dragging down.
					// Go through each item between the drag_start_index, focused item and the picked item and remove or invert their selection.
					while ( focused_index < drag_start_index && focused_index < pick_index )
					{
						if ( node != NULL )
						{
							if ( ctrl_down )
							{
								if ( node->flag & TLVS_SELECTED )
								{
									--g_selected_count;
								}
								else
								{
									++g_selected_count;
								}

								node->flag ^= TLVS_SELECTED;
							}
							else
							{
								--g_selected_count;

								node->flag = TLVS_NONE;
							}

							node = TLV_NextNode( node, false );
						}

						++focused_index;
					}
				}

				node = TLV_NextNode( node, false );

				if ( node != NULL )
				{
					// We've started our selection and dragged down.
					// Go through each item between the focused item and the picked item and add or invert their selection.
					while ( focused_index < pick_index )
					{
						if ( node != NULL )
						{
							if ( ctrl_down )
							{
								if ( node->flag & TLVS_SELECTED )
								{
									--g_selected_count;
								}
								else
								{
									++g_selected_count;
								}

								node->flag ^= TLVS_SELECTED;
							}
							else
							{
								if ( !( node->flag & TLVS_SELECTED ) )
								{
									++g_selected_count;
								}

								node->flag |= TLVS_SELECTED;
							}

							node = TLV_NextNode( node, false );
						}

						++focused_index;
					}
				}

				// Deselect the last node if we're moving to a negative index.
				if ( g_drag_start_index == -1 &&
					 pick_index >= g_expanded_item_count &&
					 g_focused_index < g_expanded_item_count )
				{
					g_focused_node = GetLastExpandedNode();

					if ( g_focused_node != NULL )
					{
						if ( ctrl_down )
						{
							if ( g_focused_node->flag & TLVS_SELECTED )
							{
								--g_selected_count;
							}
							else
							{
								++g_selected_count;
							}

							g_focused_node->flag ^= TLVS_SELECTED;
						}
						else
						{
							--g_selected_count;

							g_focused_node->flag = TLVS_NONE;
						}
					}
				}
			}

			if ( g_base_selected_node != NULL )
			{
				g_base_selected_node->flag &= ~TLVS_FOCUSED;
			}

			if ( g_focused_node != NULL )
			{
				g_focused_node->flag &= ~TLVS_FOCUSED;
			}

			g_focused_node = tli_node;
			g_focused_index = pick_index;

			if ( g_focused_node != NULL )
			{
				g_focused_node->flag |= TLVS_FOCUSED;
			}
		}

SCROLL_WINDOW:

		// Determine how much to scroll up/down/left/right based on how far we've dragged from the visible area.
		int v_scroll_index = ( cur_pos.y - g_client_rc.top ) / g_row_height;
		if ( v_scroll_index < 0 )
		{
			if ( v_scroll_index < -5 )
			{
				g_v_scroll_line_amount = 5;
			}
			else
			{
				g_v_scroll_line_amount = ( unsigned char )-v_scroll_index;
			}

			g_v_scroll_direction = 2;
		}
		else if ( v_scroll_index > ( g_visible_rows - 1 ) )
		{
			int line_amount = v_scroll_index - ( g_visible_rows - 1 );
			if ( line_amount < 5 )
			{
				g_v_scroll_line_amount = ( unsigned char )line_amount;
			}
			else
			{
				g_v_scroll_line_amount = 5;
			}

			g_v_scroll_direction = 1;
		}
		else
		{
			g_v_scroll_direction = 0;
		}

		if ( cur_pos.x < 0 )
		{
			if ( cur_pos.x <= -50 )
			{
				g_h_scroll_line_amount = 50;
			}
			else
			{
				g_h_scroll_line_amount = ( unsigned char )( 0 - cur_pos.x );
			}

			g_h_scroll_direction = 1;
		}
		else if ( cur_pos.x > g_client_rc.right )
		{
			if ( cur_pos.x >= g_client_rc.right + 50 )
			{
				g_h_scroll_line_amount = 50;
			}
			else
			{
				g_h_scroll_line_amount = ( unsigned char )( cur_pos.x - g_client_rc.right );
			}

			g_h_scroll_direction = 2;
		}
		else
		{
			g_h_scroll_direction = 0;
		}

		if ( !g_scroll_timer_active )
		{
			if ( g_v_scroll_direction != 0 || g_h_scroll_direction != 0 )
			{
				_SetTimer( hWnd, IDT_SCROLL_TIMER, SCROLL_TIMER_FREQUENCY, ( TIMERPROC )ScrollTimerProc );

				g_scroll_timer_active = true;
			}
		}
		else
		{
			if ( g_v_scroll_direction == 0 && g_h_scroll_direction == 0 )
			{
				_KillTimer( hWnd, IDT_SCROLL_TIMER );

				g_scroll_timer_active = false;
			}
		}

		// Update our selection bounds.
		if ( ctrl_down || shift_down )
		{		
			if ( cur_pos.x <= g_header_width )
			{
				if ( g_base_selected_index < g_focused_index && g_base_selected_index < g_mod_first_selection_index )
				{
					g_first_selection_node = g_base_selected_node;
					g_first_selection_index = g_base_selected_index;
				}
				else if ( g_focused_index < g_mod_first_selection_index && g_focused_index != -1 )
				{
					g_first_selection_node = g_focused_node;
					g_first_selection_index = g_focused_index;
				}
				else
				{
					g_first_selection_node = g_mod_first_selection_node;
					g_first_selection_index = g_mod_first_selection_index;
				}
			}
			else
			{
				g_first_selection_node = g_mod_first_selection_node;
				g_first_selection_index = g_mod_first_selection_index;
			}

			// Drag from bottom. Scroll up and over already selected items.
			if ( g_base_selected_index >= g_expanded_item_count )
			{
				if ( g_focused_index <= ( g_expanded_item_count - 1 ) && g_focused_index != -1 )
				{
					g_last_selection_node = GetLastExpandedNode();
					g_last_selection_index = g_expanded_item_count - 1;
				}
				else
				{
					g_last_selection_node = g_mod_last_selection_node;
					g_last_selection_index = g_mod_last_selection_index;
				}
			}
			else
			{
				// Drag from right. Scroll up and over already selected items.
				if ( g_drag_pos.x > g_header_width && g_focused_index < g_base_selected_index && g_focused_index != -1 )
				{
					if ( g_base_selected_index > g_last_selection_index )
					{
						g_last_selection_node = g_base_selected_node;
						g_last_selection_index = g_base_selected_index;
					}
				}
				else if ( g_focused_index > g_mod_last_selection_index )
				{
					g_last_selection_node = g_focused_node;
					g_last_selection_index = g_focused_index;
				}
				else
				{
					g_last_selection_node = g_mod_last_selection_node;
					g_last_selection_index = g_mod_last_selection_index;
				}
			}
		}
		else
		{
			if ( g_focused_index < g_base_selected_index )
			{
				g_first_selection_node = g_focused_node;
				g_first_selection_index = g_focused_index;

				g_last_selection_node = g_base_selected_node;
				g_last_selection_index = g_base_selected_index;
			}
			else
			{
				g_first_selection_node = g_base_selected_node;
				g_first_selection_index = g_base_selected_index;

				g_last_selection_node = g_focused_node;
				g_last_selection_index = g_focused_index;
			}
		}

		_InvalidateRect( hWnd, &g_client_rc, TRUE );
	}
}

void HandleMouseDrag( HWND hWnd )
{
	if ( g_is_dragging )
	{
		POINT cur_pos;
		_GetCursorPos( &cur_pos );
		_ScreenToClient( hWnd, &cur_pos );

		// Show the drag position line.
		if ( _abs( cur_pos.x - g_drag_pos.x ) >= 5 ||
			 _abs( cur_pos.y - g_drag_pos.y ) >= 5 )
		{
			g_draw_drag = true;
		}

		// Hide the edit textbox if it's displayed and we've clicked outside the Filename column.
		if ( g_show_edit_state != 0 )
		{
			_KillTimer( hWnd, IDT_EDIT_TIMER );

			g_show_edit_state = 0;
		}

		int pick_index = ( cur_pos.y - g_client_rc.top ) / g_row_height;
		if ( pick_index < 0 )
		{
			pick_index = 0;
		}
		pick_index += g_first_visible_index;

		// From the first visible node, get the node that was clicked.
		TREELISTNODE *tli_node = TLV_GetFirstVisibleItem();
		for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
		{
			tli_node = TLV_NextNode( tli_node, false );
		}

		g_mod_key_active = false;

		// We've moved over another index that isn't the currently focused index.
		if ( pick_index != g_focused_index )
		{
			if ( g_base_selected_node != NULL )
			{
				g_base_selected_node->flag &= ~TLVS_FOCUSED;
			}

			if ( g_focused_node != NULL )
			{
				g_focused_node->flag &= ~TLVS_FOCUSED;
			}

			g_focused_node = tli_node;
			g_focused_index = pick_index;

			if ( g_focused_node != NULL )
			{
				g_focused_node->flag |= TLVS_FOCUSED;
			}
		}

		// Determine how much to scroll up/down/left/right based on how far we've dragged from the visible area.
		int v_scroll_index = ( cur_pos.y - g_client_rc.top ) / g_row_height;
		if ( v_scroll_index < 0 )
		{
			if ( v_scroll_index < -5 )
			{
				g_v_scroll_line_amount = 5;
			}
			else
			{
				g_v_scroll_line_amount = ( unsigned char )-v_scroll_index;
			}

			g_v_scroll_direction = 2;
		}
		else if ( v_scroll_index > ( g_visible_rows - 1 ) )
		{
			int line_amount = v_scroll_index - ( g_visible_rows - 1 );
			if ( line_amount < 5 )
			{
				g_v_scroll_line_amount = ( unsigned char )line_amount;
			}
			else
			{
				g_v_scroll_line_amount = 5;
			}

			g_v_scroll_direction = 1;
		}
		else
		{
			g_v_scroll_direction = 0;
		}

		if ( cur_pos.x < 0 )
		{
			if ( cur_pos.x <= -50 )
			{
				g_h_scroll_line_amount = 50;
			}
			else
			{
				g_h_scroll_line_amount = ( unsigned char )( 0 - cur_pos.x );
			}

			g_h_scroll_direction = 1;
		}
		else if ( cur_pos.x > g_client_rc.right )
		{
			if ( cur_pos.x >= g_client_rc.right + 50 )
			{
				g_h_scroll_line_amount = 50;
			}
			else
			{
				g_h_scroll_line_amount = ( unsigned char )( cur_pos.x - g_client_rc.right );
			}

			g_h_scroll_direction = 2;
		}
		else
		{
			g_h_scroll_direction = 0;
		}

		if ( !g_scroll_timer_active )
		{
			if ( g_v_scroll_direction != 0 || g_h_scroll_direction != 0 )
			{
				_SetTimer( hWnd, IDT_SCROLL_TIMER, SCROLL_TIMER_FREQUENCY, ( TIMERPROC )ScrollTimerProc );

				g_scroll_timer_active = true;
			}
		}
		else
		{
			if ( g_v_scroll_direction == 0 && g_h_scroll_direction == 0 )
			{
				_KillTimer( hWnd, IDT_SCROLL_TIMER );

				g_scroll_timer_active = false;
			}
		}

		_InvalidateRect( hWnd, &g_client_rc, TRUE );
	}
}

void SetEditState( HWND hWnd, TREELISTNODE *tli_node )
{
	// See if we've clicked within the Filename column so that we can show the edit textbox.
	int index = GetColumnIndexFromVirtualIndex( *download_columns[ COLUMN_FILENAME ], download_columns, NUM_COLUMNS );

	// Allow the edit textbox to be shown if we've clicked within the Filename column.
	if ( index != -1 &&
		 *download_columns[ COLUMN_FILENAME ] != -1 &&
		 g_drag_start_index != -1 &&
		 g_base_selected_node == tli_node &&
		 g_base_selected_node == g_focused_node &&
		 g_base_selected_node->data_type & TLVDT_GROUP )
	{
		_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, index, ( LPARAM )&g_edit_column_rc );

		SCROLLINFO si;
		_memzero( &si, sizeof( SCROLLINFO ) );
		si.cbSize = sizeof( SCROLLINFO );
		si.fMask = SIF_POS;
		_GetScrollInfo( hWnd, SB_HORZ, &si );

		g_edit_column_rc.left -= si.nPos;
		g_edit_column_rc.right -= si.nPos;

		if ( g_drag_pos.x >= g_edit_column_rc.left && g_drag_pos.x <= g_edit_column_rc.right )
		{
			if ( g_edit_column_rc.left < 0 )
			{
				int offset = Scroll( &si, SCROLL_TYPE_LEFT, -g_edit_column_rc.left );

				g_edit_column_rc.left += offset;
				g_edit_column_rc.right += offset;

				HDWP hdwp = _BeginDeferWindowPos( 1 );
				_DeferWindowPos( hdwp, g_hWnd_tlv_header, HWND_TOP, -si.nPos, 0, g_client_rc.right + si.nPos, g_header_height, 0 );
				_EndDeferWindowPos( hdwp );

				_SetScrollInfo( hWnd, SB_HORZ, &si, TRUE );
			}

			g_show_edit_state = ( g_show_edit_state == 2 ? 0 : 1 );	// 1 = activate the edit textbox
		}
		else
		{
			index = -1;
		}
	}

	if ( index == -1 )
	{
		// Hide the edit textbox if it's displayed and we've clicked outside the Filename column.
		if ( g_show_edit_state != 0 )
		{
			_KillTimer( hWnd, IDT_EDIT_TIMER );

			g_show_edit_state = 0;
		}
	}
}

void HandleMouseClick( HWND hWnd, bool right_button )
{
	_GetCursorPos( &g_drag_pos );
	_ScreenToClient( hWnd, &g_drag_pos );

	g_drag_rc.left = g_drag_rc.right = g_drag_pos.x;
	g_drag_rc.top = g_drag_rc.bottom = g_drag_pos.y;

	int pick_index = ( g_drag_pos.y - g_client_rc.top ) / g_row_height;
	if ( pick_index < 0 )
	{
		pick_index = 0;
	}
	pick_index += g_first_visible_index;

	// From the first visible node, get the node that was clicked.
	TREELISTNODE *tli_node = TLV_GetFirstVisibleItem();
	for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
	{
		tli_node = TLV_NextNode( tli_node, false );
	}

	// Handle clicking on the +/- glyph
	if ( !right_button && tli_node != NULL && tli_node->child_count > 0 )
	{
		int glyph_top = g_client_rc.top + ( ( pick_index - g_first_visible_index ) * g_row_height ) + ( ( g_row_height - g_glyph_size.cy ) / 2 ) + _SCALE_TLV_( 1 );

		if ( ( g_drag_pos.x >= g_glyph_offset && g_drag_pos.x < g_glyph_offset + g_glyph_size.cx ) &&
			 ( g_drag_pos.y >= glyph_top && g_drag_pos.y < glyph_top + g_glyph_size.cy ) )
		{
			TLV_ExpandCollapseParent( tli_node, pick_index, !tli_node->is_expanded );

			HandleWindowChange( hWnd );

			_InvalidateRect( hWnd, &g_client_rc, TRUE );

			return;	// Don't want to update the menus.
		}
	}

	// Handle right clicking on an item.
	if ( right_button && tli_node != NULL && tli_node->flag & TLVS_SELECTED )
	{
		g_focused_node = g_base_selected_node = tli_node;
		g_focused_index = g_base_selected_index = pick_index;

		goto MOUSE_CLICK_END;
	}

	// See if any modifier key is pressed.
	bool ctrl_down = ( _GetKeyState( VK_CONTROL ) & 0x8000 ) ? true : false;
	bool shift_down = ( _GetKeyState( VK_SHIFT ) & 0x8000 ) && !ctrl_down ? true : false;

	// Handle a mouse click when we're in the list edit mode.
	if ( g_in_list_edit_mode && ( !ctrl_down && !shift_down ) &&
		 g_drag_start_index != -1 && g_drag_pos.x <= g_header_width &&
		 tli_node != NULL && ( tli_node->flag & TLVS_SELECTED ) )
	{
		if ( g_focused_node != NULL )
		{
			g_focused_node->flag &= ~TLVS_FOCUSED;
		}

		g_focused_node = tli_node;
		g_focused_index = pick_index;

		g_focused_node->flag |= TLVS_FOCUSED;

		SetEditState( hWnd, tli_node );

		goto MOUSE_CLICK_DRAG;
	}

	// Remove the selection flag for all items in the list.
	TLV_ClearSelected( ctrl_down, shift_down );

	if ( g_selected_count == 0 )
	{
		TLV_ResetSelectionBounds();
	}

	if ( g_focused_node != NULL )
	{
		g_focused_node->flag &= ~TLVS_FOCUSED;
	}

	// We've clicked a row beyond the width of the header (passed the last column).
	if ( g_drag_pos.x > g_header_width )
	{
		// Hide the edit control if it's visible.
		if ( g_show_edit_state != 0 )
		{
			_KillTimer( hWnd, IDT_EDIT_TIMER );

			g_show_edit_state = 0;
		}

		if ( !ctrl_down && !shift_down )
		{
			g_selected_count = 0;
		}

		g_drag_start_index = -1;

		g_focused_node = NULL;
		g_focused_index = -1;

		g_base_selected_node = tli_node;
		g_base_selected_index = pick_index;
	}
	else	// We've clicked a row within the width of the header.
	{
		// Assuming it's a valid item.
		if ( tli_node != NULL )
		{
			// The clicked item will be the new focused item.
			tli_node->flag |= TLVS_FOCUSED;

			// If ctrl was held down, then invert the item's selection state.
			if ( ctrl_down && !shift_down )
			{
				if ( tli_node->flag & TLVS_SELECTED )
				{
					--g_selected_count;
				}
				else
				{
					++g_selected_count;
				}

				tli_node->flag ^= TLVS_SELECTED;
			}
			else if ( shift_down && !ctrl_down )	// If shift was held down, then select all items between the last focused node, and the newly focused node.
			{
				if ( !( tli_node->flag & TLVS_SELECTED ) )
				{
					tli_node->flag |= TLVS_SELECTED;	// Selected

					++g_selected_count;
				}

				if ( g_focused_index != -1 )
				{
					TREELISTNODE *node = g_focused_node;
					int index = g_focused_index;

					while ( index < pick_index )
					{
						if ( node != NULL )
						{
							if ( !( node->flag & TLVS_SELECTED ) )
							{
								node->flag |= TLVS_SELECTED;

								++g_selected_count;
							}

							node = TLV_NextNode( node, false );
						}

						++index;
					}

					while ( index > pick_index )
					{
						if ( node != NULL )
						{
							if ( !( node->flag & TLVS_SELECTED ) )
							{
								node->flag |= TLVS_SELECTED;

								++g_selected_count;
							}

							node = TLV_PrevNode( node, false );
						}

						--index;
					}
				}
			}
			else	// No modifier key was held down.
			{
				tli_node->flag |= TLVS_SELECTED;

				g_selected_count = 1;

				SetEditState( hWnd, tli_node );

				TLV_ResetSelectionBounds();
			}

			g_drag_start_index = pick_index;

			g_focused_node = tli_node;
			g_focused_index = pick_index;

			// We don't want to update the base selected node if shift is held down.
			if ( g_base_selected_node == NULL || !shift_down )
			{
				g_base_selected_node = tli_node;
				g_base_selected_index = pick_index;
			}
		}
		else	// A row was selected, but there's no item there.
		{
			// Hide the edit textbox if it's displayed and we've clicked outside the Filename column.
			if ( g_show_edit_state != 0 )
			{
				_KillTimer( hWnd, IDT_EDIT_TIMER );

				g_show_edit_state = 0;
			}

			// If no modifier key was held down, then nothing will be selected.
			if ( !ctrl_down && !shift_down )
			{
				g_selected_count = 0;
			}

			g_drag_start_index = -1;

			g_focused_node = tli_node;
			g_focused_index = pick_index;

			g_base_selected_node = tli_node;
			g_base_selected_index = pick_index;
		}
	}

	// Update our selection bounds.
	if ( g_focused_index != -1 )
	{
		TLV_SetSelectionBounds( g_focused_index, g_focused_node );
		TLV_SetSelectionBounds( g_base_selected_index, g_base_selected_node );
	}

	// These are the selection bounds when we have a modifier key held down.
	if ( ctrl_down || shift_down )
	{
		if ( !g_mod_key_active )
		{
			g_mod_key_active = true;

			g_mod_first_selection_node = g_first_selection_node;
			g_mod_first_selection_index = g_first_selection_index;

			g_mod_last_selection_node = g_last_selection_node;
			g_mod_last_selection_index = g_last_selection_index;
		}
	}
	else
	{
		g_mod_key_active = false;
	}

	// Prevent children and deselected items from being dragged.
	if ( g_in_list_edit_mode && g_focused_node != NULL && ( g_focused_node->parent != NULL || !( g_focused_node->flag & TLVS_SELECTED ) ) )
	{
		goto MOUSE_CLICK_END;
	}

MOUSE_CLICK_DRAG:

	// Assume we're dragging until we release the mouse button.
	_SetCapture( hWnd );
	g_is_dragging = true;

MOUSE_CLICK_END:

	_InvalidateRect( hWnd, &g_client_rc, TRUE );

	if ( !in_worker_thread )
	{
		UpdateMenus( true );
	}
}

void HandleRename( TREELISTNODE *tln )
{
	if ( tln != NULL && tln->data != NULL )
	{
		unsigned int filename_length = ( unsigned int )_SendMessageW( g_hWnd_edit_box, WM_GETTEXTLENGTH, 0, 0 );

		// Prevent the edit if the text length is 0.
		if ( filename_length > 0 )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
				RENAME_INFO *ri = ( RENAME_INFO * )GlobalAlloc( GPTR, sizeof( RENAME_INFO ) );
				if ( ri != NULL )
				{
					ri->di = di;
					ri->filename_length = filename_length;
					ri->filename = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( ri->filename_length + 1 ) );
					_SendMessageW( g_hWnd_edit_box, WM_GETTEXT, ri->filename_length + 1, ( LPARAM )ri->filename );

					// ri is freed in rename_file.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, rename_file, ( void * )ri, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						GlobalFree( ri->filename );
						GlobalFree( ri );
					}
				}
			}
		}
	}
}

LRESULT CALLBACK EditBoxSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static bool ignore_kill_focus = false;

	switch ( msg )
	{
		case WM_GETDLGCODE:
		//case WM_KEYDOWN:
		{
			if ( wParam == VK_RETURN )
			{
				ignore_kill_focus = true;

				HandleRename( g_base_selected_node );

				_DestroyWindow( hWnd );

				//return 0;
			}
			else if ( wParam == VK_ESCAPE )
			{
				ignore_kill_focus = true;

				_DestroyWindow( hWnd );

				//return 0;
			}

			return DLGC_WANTALLKEYS;
		}
		break;

		case WM_KILLFOCUS:
		{
			if ( !ignore_kill_focus )
			{
				HandleRename( g_base_selected_node );
			}

			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			//_SetFocus( _GetParent( hWnd ) );

			ignore_kill_focus = false;

			g_hWnd_edit_box = NULL;

			return 0;
		}
		break;
	}

	return _CallWindowProcW( EditBoxProc, hWnd, msg, wParam, lParam );
}

void CreateEditBox( HWND hWnd )
{
	g_show_edit_state = 2;	// Edit control is active.

	g_hWnd_edit_box = _CreateWindowW( WC_EDIT, L"", ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_BORDER | WS_VISIBLE, g_edit_column_rc.left, g_client_rc.top + ( ( g_base_selected_index - g_first_visible_index ) * g_row_height ) + 1, g_edit_column_rc.right - g_edit_column_rc.left, g_row_height - 1, hWnd, ( HMENU )EDIT_BOX, NULL, NULL );
	_SendMessageW( g_hWnd_edit_box, WM_SETFONT, ( WPARAM )g_hFont, 0 );

	EditBoxProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_edit_box, GWLP_WNDPROC );
	_SetWindowLongPtrW( g_hWnd_edit_box, GWLP_WNDPROC, ( LONG_PTR )EditBoxSubProc );

#ifdef ENABLE_DARK_MODE
	if ( g_use_dark_mode )
	{
		EnumChildProc( g_hWnd_edit_box, NULL );
		EnumTLWProc( hWnd, NULL );
	}
#endif

	if ( g_base_selected_node != NULL && g_base_selected_node->data != NULL )
	{
		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )g_base_selected_node->data;

		// Set our edit control's text to the list item's text.
		_SendMessageW( g_hWnd_edit_box, WM_SETTEXT, NULL, ( LPARAM )( di->shared_info->file_path + di->shared_info->filename_offset ) );

		// Get the length of the filename without the extension.
		int ext_len = lstrlenW( di->shared_info->file_path + di->shared_info->filename_offset );
		while ( ext_len != 0 && ( di->shared_info->file_path + di->shared_info->filename_offset )[ --ext_len ] != L'.' );

		// Select all the text except the file extension (if ext_len = 0, then everything is selected)
		if ( ext_len == 0 ) { --ext_len; }
		_SendMessageW( g_hWnd_edit_box, EM_SETSEL, 0, ext_len );

		// Limit the length of the filename so that the file directory + filename + NULL isn't greater than MAX_PATH.
		_SendMessageW( g_hWnd_edit_box, EM_LIMITTEXT, MAX_PATH - ( di->shared_info->filename_offset + 1 ), 0 );
	}

	_SetFocus( g_hWnd_edit_box );
}

VOID CALLBACK EditTimerProc( HWND hWnd, UINT /*msg*/, UINT /*idTimer*/, DWORD /*dwTime*/ )
{
	CreateEditBox( hWnd );

	_KillTimer( hWnd, IDT_EDIT_TIMER );
}

struct SEARCHRESULT
{
	TREELISTNODE *node;
	int index;
};

SEARCHRESULT SearchList( TREELISTNODE *start, TREELISTNODE *end, wchar_t *text, DWORD text_length )
{
	SEARCHRESULT sr;
	_memzero( &sr, sizeof( SEARCHRESULT ) );

	while ( start != end )
	{
		DOWNLOAD_INFO *di = ( ( DOWNLOAD_INFO * )start->data )->shared_info;

		if ( _StrCmpNIW( di->file_path + di->filename_offset, text, text_length ) == 0 )
		{
			sr.node = start;

			break;
		}

		++sr.index;
		start = start->next;
	}

	return sr;
}

void __DrawTextW( HDC hdc, int x, int y, UINT options, const RECT *lprect, LPCWSTR lpString, UINT c )
{
	SIZE size;
	int hpos, vpos;
	RECT rc;
	int num_fit;

	int width = lprect->right - lprect->left;

	if ( c > 0 && _GetTextExtentExPointW( hdc, lpString, c, width, &num_fit, NULL, &size ) == TRUE )
	{
		SIZE e_size;
		e_size.cx = 0;

		SIZE adj_size;
		adj_size.cx = size.cx;

		// String doesn't fit within our bounding rect. We need to add an ellipsis.
		if ( size.cx > width )
		{
			// Measure the size of our ellipsis.
			if ( _GetTextExtentExPointW( hdc, ELLIPSISW, ELLIPSISW_LENGTH, width, NULL, NULL, &e_size ) == TRUE )
			{
				if ( e_size.cx > width )
				{
					e_size.cx = width;
				}

				// Remove the width of the ellipsis from our bounding rect.
				// This leaves us with the width we have to fit our string into.
				int adj_width = width - e_size.cx;
				adj_size.cx = adj_width;

				// If we have some amount of characters that fit into the original bounding rect, then find out how many can fit into the adjusted bounding rect.
				while ( num_fit > 0 )
				{
					if ( _GetTextExtentExPointW( hdc, lpString, num_fit, adj_width, NULL, NULL, &adj_size ) == FALSE )
					{
						break;
					}

					if ( adj_size.cx <= adj_width )
					{
						break;
					}

					--num_fit;
				}
			}
		}

		// Align our text accordingly.
		if ( options & _DT_RIGHT )
		{
			if ( num_fit == 0 )
			{
				adj_size.cx = 0;
			}

			hpos = width - ( adj_size.cx + e_size.cx );
		}
		else if ( options & _DT_CENTER )
		{
			if ( num_fit == 0 )
			{
				num_fit = 1;
			}

			hpos = ( width - ( adj_size.cx + e_size.cx ) ) / 2;

			e_size.cx = width - adj_size.cx;
		}
		else// if ( options & _DT_LEFT )
		{
			if ( num_fit == 0 )
			{
				num_fit = 1;

				if ( adj_size.cx <= width - e_size.cx )
				{
					adj_size.cx = width;
				}
			}

			hpos = 0;

			e_size.cx = width - adj_size.cx;
		}

		if ( options & _DT_VCENTER )
		{
			vpos = ( ( lprect->bottom - lprect->top ) - size.cy ) / 2;
		}
		else
		{
			vpos = 0;
		}

		rc.top = lprect->top;
		rc.bottom = lprect->bottom;
		rc.left = lprect->left;
		rc.right = rc.left + hpos + adj_size.cx;

		// Print the characters that fit into our adjusted bounding rect.
		if ( num_fit > 0 )
		{
			_ExtTextOutW( hdc, x + hpos, y + vpos, ETO_CLIPPED, &rc, lpString, num_fit, NULL );
		}

		// Print the ellipsis into its bounding rect.
		if ( size.cx > width )
		{
			rc.left = rc.right;
			rc.right += e_size.cx;

			_ExtTextOutW( hdc, x + hpos + adj_size.cx, y + vpos, ETO_CLIPPED, &rc, ELLIPSISW, ELLIPSISW_LENGTH, NULL );
		}
	}
}

void DrawTreeListView( HWND hWnd )
{
	if ( g_skip_list_draw )
	{
		return;
	}

	PAINTSTRUCT ps;
	HDC hDC = _BeginPaint( hWnd, &ps );

	RECT rc;
	_GetClientRect( hWnd, &rc );	// Includes the area occupied by the header control.

	// Create a memory buffer to draw to.
	HDC hdcMem = _CreateCompatibleDC( hDC );

	// Create a bitmap in our memory buffer that's the size of our window.
	if ( g_size_changed )
	{
		g_size_changed = false;

		_DeleteObject( g_hbm );
		g_hbm = NULL;
	}

	// This is cached. We'll delete it when we destory the control.
	if ( g_hbm == NULL )
	{
		g_hbm = _CreateCompatibleBitmap( hDC, rc.right - rc.left, rc.bottom - rc.top );
	}

	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, g_hbm );
	_DeleteObject( ohbm );

	// Fill the memory background with the window color
	HBRUSH background = _CreateSolidBrush( cfg_background_color );
	_FillRect( hdcMem, &rc, background );
	_DeleteObject( background );

	int row_count = 0;

	RECT rc_array[ NUM_COLUMNS ];

	char start_index = -1;
	char end_index = -1;

	RECT row_rc;
	row_rc.left = 0;
	row_rc.right = 0;
	row_rc.top = 0;
	row_rc.bottom = g_client_rc.top;

	SCROLLINFO si;
	_memzero( &si, sizeof( SCROLLINFO ) );
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_POS | SIF_TRACKPOS;
	_GetScrollInfo( hWnd, SB_HORZ, &si );

	int arr[ NUM_COLUMNS ];
	int arr2[ NUM_COLUMNS ];

	_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );

	_memcpy_s( arr2, sizeof( int ) * NUM_COLUMNS, arr, sizeof( int ) * NUM_COLUMNS );

	// Offset the virtual indices to match the actual index.
	OffsetVirtualIndices( arr2, download_columns, NUM_COLUMNS, g_total_columns );

	for ( unsigned char i = 0; i < g_total_columns; ++i )
	{
		// Get the width of the column header.
		_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, arr[ i ], ( LPARAM )&rc_array[ i ] );

		// Skip drawing columns to the left of the window that aren't visible.
		if ( rc_array[ i ].right <= si.nPos )
		{
			continue;	// But continue to see if the next column is visible.
		}

		rc_array[ i ].left -= si.nPos;
		rc_array[ i ].right -= si.nPos;

		// Skip drawing columns to the right of the window that aren't visible.
		if ( rc_array[ i ].left >= g_client_rc.right )
		{
			break;	// No column that remains will be visible. So we can exit the loop.
		}

		if ( start_index == -1 )
		{
			start_index = i;

			row_rc.left = rc_array[ i ].left;
		}

		end_index = i;

		row_rc.right = rc_array[ i ].right;
	}

	row_count = 0;

	int node_count = g_first_visible_index;
	int root_count = g_first_visible_root_index + 1;
	int child_count = g_first_visible_index - TLV_GetParentIndex( g_first_visible_node, g_first_visible_index );

	int gridline_offset = ( cfg_show_gridlines ? 1 : 0 );

	TREELISTNODE *tln = TLV_GetFirstVisibleItem();
	while ( tln != NULL )
	{
		row_rc.top = row_rc.bottom;
		row_rc.bottom += g_row_height;

		RECT row_bg_rc;
		row_bg_rc.top = row_rc.top + gridline_offset;
		row_bg_rc.left = row_rc.left;
		row_bg_rc.right = ( cfg_draw_full_rows ? g_client_rc.right : row_rc.right );
		row_bg_rc.bottom = row_rc.bottom;

		// Draw odd/even/highlight rectangle.
		bool selected;
		HBRUSH color;
		if ( tln->flag & TLVS_SELECTED )
		{
			color = _CreateSolidBrush( ( node_count & 1 ? cfg_even_row_highlight_color : cfg_odd_row_highlight_color ) );
			selected = true;
		}
		else
		{
			color = _CreateSolidBrush( ( node_count & 1 ? cfg_even_row_background_color : cfg_odd_row_background_color ) );
			selected = false;
		}
		_FillRect( hdcMem, &row_bg_rc, color );
		_DeleteObject( color );

		HFONT ohf = ( HFONT )_SelectObject( hdcMem, ( node_count & 1 ? tlv_even_row_font_settings.font : tlv_odd_row_font_settings.font ) );
		if ( ohf != tlv_even_row_font_settings.font && ohf != tlv_odd_row_font_settings.font )
		{
			_DeleteObject( ohf );
		}

		// Transparent background for text.
		_SetBkMode( hdcMem, TRANSPARENT );

		// Draw the +/- glyph.
		if ( start_index == 0 )
		{
			if ( tln->child != NULL )
			{
				if ( rc_array[ 0 ].right >= ( g_glyph_offset + g_glyph_size.cx + _SCALE_TLV_( 2 ) ) )
				{
					if ( g_hTheme != NULL )
					{
						RECT glyph_rc;
						glyph_rc.top = row_rc.top + ( ( g_row_height - g_glyph_size.cy ) / 2 ) + gridline_offset;
						glyph_rc.bottom = glyph_rc.top + g_glyph_size.cy;//row_rc.bottom;

						glyph_rc.left = rc_array[ 0 ].left + g_glyph_offset;
						glyph_rc.right = rc_array[ 0 ].left + g_glyph_offset + g_glyph_size.cx;

						_DrawThemeBackground( g_hTheme, hdcMem, TVP_GLYPH, ( tln->is_expanded ? GLPS_OPENED : GLPS_CLOSED ), &glyph_rc, 0 );
					}
				}
			}
		}

		// Get the item's text.
		wchar_t tbuf[ 128 ];
		wchar_t *buf = tbuf;

		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
		if ( di != NULL )
		{
			for ( char i = start_index; i <= end_index; ++i )
			{
				RECT item_rc;
				item_rc.top = row_rc.top;
				item_rc.bottom = row_rc.bottom;
				item_rc.left = rc_array[ i ].left;
				item_rc.right = rc_array[ i ].right;

				RECT text_rc;

				if ( i == 0 )
				{
					text_rc.left = item_rc.left + g_glyph_offset + g_glyph_size.cx + _SCALE_TLV_( 3 );
				}
				else
				{
					text_rc.left = item_rc.left + _SCALE_TLV_( 5 );
				}

				text_rc.right = item_rc.right - _SCALE_TLV_( 5 );
				if ( text_rc.right < text_rc.left )
				{
					text_rc.right = text_rc.left;
				}
				text_rc.top = item_rc.top + gridline_offset;
				text_rc.bottom = item_rc.bottom;

				// Save the appropriate text in our buffer for the current column.
				buf = GetDownloadInfoString( di, arr2[ i ], root_count, child_count, tbuf, 128 );

				if ( buf == NULL )
				{
					tbuf[ 0 ] = L'\0';
					buf = tbuf;
				}

				unsigned int align = 0;	// 0 = Left, 1, = Right, 2 = Center

				switch ( arr2[ i ] )
				{
					case COLUMN_NUM:
					case COLUMN_DATE_AND_TIME_ADDED:
					case COLUMN_CATEGORY:
					case COLUMN_COMMENTS:
					case COLUMN_DOWNLOAD_DIRECTORY:
					case COLUMN_FILENAME:
					case COLUMN_SSL_TLS_VERSION:
					case COLUMN_URL:
					{
						align = _DT_LEFT;
					}
					break;

					case COLUMN_ACTIVE_PARTS:
					case COLUMN_DOWNLOAD_SPEED:
					case COLUMN_DOWNLOAD_SPEED_LIMIT:
					case COLUMN_DOWNLOADED:
					case COLUMN_FILE_SIZE:
					case COLUMN_TIME_ELAPSED:
					case COLUMN_TIME_REMAINING:
					{
						align = _DT_RIGHT;
					}
					break;

					/*case COLUMN_FILE_TYPE:
					{
					}
					break;*/

					case COLUMN_PROGRESS:
					{
						align = _DT_CENTER;
					}
					break;
				}

				int buf_length = lstrlenW( buf );

				if ( arr2[ i ] == COLUMN_FILE_TYPE )	// File Type
				{
					if ( tln->data_type & TLVDT_GROUP && di->shared_info->icon != NULL )
					{
						int icon_top_offset = g_row_height - _SCALE_TLV_( 18 );
						if ( icon_top_offset > 0 )
						{
							if ( icon_top_offset & 1 )
							{
								//++icon_top_offset;
								icon_top_offset = icon_top_offset + _SCALE_TLV_( 1 );
							}

							icon_top_offset /= 2;
						}
						else
						{
							icon_top_offset = _SCALE_TLV_( 1 );
						}

						RECT icon_rc;
						icon_rc.top = item_rc.top + gridline_offset;
						icon_rc.bottom = item_rc.bottom - _SCALE_TLV_( 1 );
						icon_rc.left = item_rc.left + _SCALE_TLV_( 2 );
						if ( i == 0 )
						{
							icon_rc.left += ( g_glyph_offset + g_glyph_size.cx );
						}
						icon_rc.right = item_rc.right - _SCALE_TLV_( 1 );
						if ( icon_rc.right < icon_rc.left )
						{
							icon_rc.right = icon_rc.left;
						}

						int icon_width = icon_rc.right - icon_rc.left;
						int icon_height = icon_rc.bottom - ( icon_rc.top + icon_top_offset );

						// Create a memory buffer to draw to.
						HDC hdcMem2 = _CreateCompatibleDC( hDC );

						HBITMAP hbm = _CreateCompatibleBitmap( hDC, icon_width, icon_height );

						ohbm = ( HBITMAP )_SelectObject( hdcMem2, hbm );
						_DeleteObject( ohbm );

						RECT icon_color_rc;
						icon_color_rc.left = 0;
						icon_color_rc.top = 0;
						icon_color_rc.right = icon_width;
						icon_color_rc.bottom = icon_height;

						if ( tln->flag & TLVS_SELECTED )
						{
							color = _CreateSolidBrush( ( node_count & 1 ? cfg_even_row_highlight_color : cfg_odd_row_highlight_color ) );
						}
						else
						{
							color = _CreateSolidBrush( ( node_count & 1 ? cfg_even_row_background_color : cfg_odd_row_background_color ) );
						}
						_FillRect( hdcMem2, &icon_color_rc, color );
						_DeleteObject( color );

						_DrawIconEx( hdcMem2, 0, 0, *di->shared_info->icon, _SCALE_TLV_( 16 ), _SCALE_TLV_( 16 ), NULL, NULL, DI_NORMAL );

						_BitBlt( hdcMem, icon_rc.left, icon_rc.top + icon_top_offset, icon_width, icon_height, hdcMem2, 0, 0, SRCCOPY );

						_DeleteObject( hbm );
						_DeleteDC( hdcMem2 );
					}
				}
				else if ( arr2[ i ] == COLUMN_PROGRESS )	// Progress
				{
					RECT progress_rc;
					progress_rc.top = item_rc.top + _SCALE_TLV_( 2 ) + gridline_offset;
					progress_rc.bottom = item_rc.bottom - _SCALE_TLV_( 2 );
					progress_rc.left = item_rc.left + _SCALE_TLV_( 2 );
					if ( i == 0 )
					{
						progress_rc.left += ( g_glyph_offset + g_glyph_size.cx );
					}
					progress_rc.right = item_rc.right - _SCALE_TLV_( 2 );
					if ( progress_rc.right < progress_rc.left )
					{
						progress_rc.right = progress_rc.left;
					}

					COLORREF color_ref_body = 0, color_ref_border = 0, color_ref_background = 0, color_ref_body_text = 0, color_ref_background_text = 0;

					if		( di->status == STATUS_CONNECTING )					{ color_ref_body = cfg_color_4a; color_ref_background = cfg_color_4b; color_ref_body_text = cfg_color_4c; color_ref_background_text = cfg_color_4d; color_ref_border = cfg_color_4e; }
					else if ( IS_STATUS( di->status, STATUS_RESTART ) )			{ color_ref_body = cfg_color_13a; color_ref_background = cfg_color_13b; color_ref_body_text = cfg_color_13c; color_ref_background_text = cfg_color_13d; color_ref_border = cfg_color_13e; }
					else if ( IS_STATUS( di->status, STATUS_PAUSED ) )			{ color_ref_body = cfg_color_10a; color_ref_background = cfg_color_10b; color_ref_body_text = cfg_color_10c; color_ref_background_text = cfg_color_10d; color_ref_border = cfg_color_10e; }
					else if ( IS_STATUS( di->status, STATUS_QUEUED ) )			{ color_ref_body = cfg_color_12a; color_ref_background = cfg_color_12b; color_ref_body_text = cfg_color_12c; color_ref_background_text = cfg_color_12d; color_ref_border = cfg_color_12e; }
					else if ( di->status == STATUS_COMPLETED )					{ color_ref_body = cfg_color_3a; color_ref_background = cfg_color_3b; color_ref_body_text = cfg_color_3c; color_ref_background_text = cfg_color_3d; color_ref_border = cfg_color_3e; }
					else if ( di->status == STATUS_STOPPED )					{ color_ref_body = cfg_color_15a; color_ref_background = cfg_color_15b; color_ref_body_text = cfg_color_15c; color_ref_background_text = cfg_color_15d; color_ref_border = cfg_color_15e; }
					else if ( di->status == STATUS_TIMED_OUT )					{ color_ref_body = cfg_color_16a; color_ref_background = cfg_color_16b; color_ref_body_text = cfg_color_16c; color_ref_background_text = cfg_color_16d; color_ref_border = cfg_color_16e; }
					else if ( di->status == STATUS_FAILED )						{ color_ref_body = cfg_color_6a; color_ref_background = cfg_color_6b; color_ref_body_text = cfg_color_6c; color_ref_background_text = cfg_color_6d; color_ref_border = cfg_color_6e; }
					else if ( di->status == STATUS_FILE_IO_ERROR )				{ color_ref_body = cfg_color_7a; color_ref_background = cfg_color_7b; color_ref_body_text = cfg_color_7c; color_ref_background_text = cfg_color_7d; color_ref_border = cfg_color_7e; }
					else if ( di->status == STATUS_SKIPPED )					{ color_ref_body = cfg_color_14a; color_ref_background = cfg_color_14b; color_ref_body_text = cfg_color_14c; color_ref_background_text = cfg_color_14d; color_ref_border = cfg_color_14e; }
					else if ( di->status == STATUS_AUTH_REQUIRED )				{ color_ref_body = cfg_color_2a; color_ref_background = cfg_color_2b; color_ref_body_text = cfg_color_2c; color_ref_background_text = cfg_color_2d; color_ref_border = cfg_color_2e; }
					else if ( di->status == STATUS_PROXY_AUTH_REQUIRED )		{ color_ref_body = cfg_color_11a; color_ref_background = cfg_color_11b; color_ref_body_text = cfg_color_11c; color_ref_background_text = cfg_color_11d; color_ref_border = cfg_color_11e; }
					else if	( di->status == STATUS_ALLOCATING_FILE )			{ color_ref_body = cfg_color_1a; color_ref_background = cfg_color_1b; color_ref_body_text = cfg_color_1c; color_ref_background_text = cfg_color_1d; color_ref_border = cfg_color_1e; }
					else if	( di->status == STATUS_MOVING_FILE )				{ color_ref_body = cfg_color_9a; color_ref_background = cfg_color_9b; color_ref_body_text = cfg_color_9c; color_ref_background_text = cfg_color_9d; color_ref_border = cfg_color_9e; }
					else if ( di->status == STATUS_INSUFFICIENT_DISK_SPACE )	{ color_ref_body = cfg_color_8a; color_ref_background = cfg_color_8b; color_ref_body_text = cfg_color_8c; color_ref_background_text = cfg_color_8d; color_ref_border = cfg_color_8e; }
					else														{ color_ref_body = cfg_color_5a; color_ref_background = cfg_color_5b; color_ref_body_text = cfg_color_5c; color_ref_background_text = cfg_color_5d; color_ref_border = cfg_color_5e; }

					int progress_width = progress_rc.right - progress_rc.left;
					int progress_height = progress_rc.bottom - progress_rc.top;

					color = _CreateSolidBrush( color_ref_background );
					_FillRect( hdcMem, &progress_rc, color );
					_DeleteObject( color );

					_SetTextColor( hdcMem, color_ref_background_text );
					__DrawTextW( hdcMem, progress_rc.left, progress_rc.top, align | _DT_VCENTER, &progress_rc, buf, buf_length );

					////////////////////

					// Create a memory buffer to draw to.
					HDC hdcMem2 = _CreateCompatibleDC( hDC );

					HBITMAP hbm = _CreateCompatibleBitmap( hDC, progress_width, progress_height );

					ohbm = ( HBITMAP )_SelectObject( hdcMem2, hbm );
					_DeleteObject( ohbm );

					ohf = ( HFONT )_SelectObject( hdcMem2, ( node_count & 1 ? tlv_even_row_font_settings.font : tlv_odd_row_font_settings.font ) );
					if ( ohf != tlv_even_row_font_settings.font && ohf != tlv_odd_row_font_settings.font )
					{
						_DeleteObject( ohf );
					}

					// Transparent background for text.
					_SetBkMode( hdcMem2, TRANSPARENT );

					if ( cfg_show_part_progress &&
					   ( IS_STATUS( di->status,
							STATUS_CONNECTING |
							STATUS_DOWNLOADING |
							STATUS_STOPPED |
							STATUS_QUEUED ) ||
							di->status == STATUS_NONE ) &&
						di->print_range_list != NULL && di->parts > 1 )
					{
						RECT progress_parts_rc;
						progress_parts_rc.top = 0;
						progress_parts_rc.left = 0;
						progress_parts_rc.right = progress_width;
						progress_parts_rc.bottom = progress_height;

						// We'll pick out sections of this bitmap to paint the progress bar.
						color = _CreateSolidBrush( color_ref_body );
						_FillRect( hdcMem2, &progress_parts_rc, color );
						_DeleteObject( color );

						RECT parts_text_rc;
						parts_text_rc.left = 0;
						parts_text_rc.right = progress_rc.right - progress_rc.left;
						parts_text_rc.top = 0;
						parts_text_rc.bottom = progress_rc.bottom - progress_rc.top;

						_SetTextColor( hdcMem2, color_ref_body_text );
						__DrawTextW( hdcMem2, 0, 0, align | _DT_VCENTER, &parts_text_rc, buf, buf_length );

						unsigned short range_info_count = 0;
						unsigned short total_range_info_count = ( di->hosts > 0 ? di->hosts : di->parts );

						unsigned long long last_range_end = 0;
						unsigned long long last_host_end = di->file_size;

						// If our hosts in a group have multiple parts, then we need to figure out what the previous offset is.
						if ( di->shared_info->host_list != &di->shared_info_node &&
							 di->shared_info_node.prev != NULL && di->shared_info_node.prev->data != NULL )
						{
							DOWNLOAD_INFO *last_di = ( DOWNLOAD_INFO * )di->shared_info_node.prev->data;
							if ( last_di->range_list != NULL )
							{
								RANGE_INFO *last_ri = ( RANGE_INFO * )( last_di->range_list->prev != NULL ? last_di->range_list->prev->data : last_di->range_list->data );
								if ( last_ri != NULL )
								{
									last_host_end = last_ri->range_end + 1;
								}
							}
						}

						DoublyLinkedList *range_node = di->print_range_list;

						// Determine the number of ranges that still need downloading.
						while ( range_node != di->range_list_end && range_node->data != NULL )
						{
							++range_info_count;

							unsigned long long range_end;
							unsigned long long range_size;
							unsigned long long content_offset;

							// We're a group item in the list.
							if ( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
							{
								DOWNLOAD_INFO *di_host = ( DOWNLOAD_INFO * )range_node->data;

								range_end = last_range_end + di_host->file_size;
								range_size = di_host->file_size;
								content_offset = di_host->downloaded;
							}
							else	// We're a host (either in a group or alone).
							{
								RANGE_INFO *ri = ( RANGE_INFO * )range_node->data;

								if ( ri->range_start >= last_host_end )
								{
									range_end = ( ri->range_end + 1 ) - last_host_end;
									range_size = ( range_end - last_range_end );
									content_offset = ri->content_offset;

									if ( ( ri->range_start - last_host_end ) > last_range_end )
									{
										content_offset += ( ( ri->range_start - last_host_end ) - last_range_end );
									}
								}
								else
								{
									range_end = ri->range_end + 1;
									range_size = ( range_end - last_range_end );
									content_offset = ri->content_offset;

									if ( ri->range_start > last_range_end )
									{
										content_offset += ( ri->range_start - last_range_end );
									}
								}
							}

							int range_offset = 0;
							int range_width = 0;
							if ( di->file_size > 0 )
							{
#ifdef _WIN64
								range_offset = ( int )_ceil( ( double )progress_width * ( ( double )last_range_end / ( double )di->file_size ) );
								range_width = ( int )_ceil( ( double )progress_width * ( ( double )range_size / ( double )di->file_size ) );
#else
								double f_range_offset = _ceil( ( double )progress_width * ( ( double )last_range_end / ( double )di->file_size ) );
								__asm
								{
									fld f_range_offset;	//; Load the floating point value onto the FPU stack.
									fistp range_offset;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}

								double f_range_width = _ceil( ( double )progress_width * ( ( double )range_size / ( double )di->file_size ) );
								__asm
								{
									fld f_range_width;	//; Load the floating point value onto the FPU stack.
									fistp range_width;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}
#endif
							}

							int range_progress_offset = 0;
							if ( range_size > 0 )
							{
#ifdef _WIN64
								range_progress_offset = ( int )_ceil( ( double )range_width * ( ( double )content_offset / ( double )range_size ) );
#else
								double f_range_progress_offset = _ceil( ( double )range_width * ( ( double )content_offset / ( double )range_size ) );
								__asm
								{
									fld f_range_progress_offset;	//; Load the floating point value onto the FPU stack.
									fistp range_progress_offset;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}
#endif
							}

							_BitBlt( hdcMem, progress_rc.left + range_offset, progress_rc.top, range_progress_offset, progress_height, hdcMem2, range_offset, 0, SRCCOPY );

							last_range_end = range_end;

							range_node = range_node->next;
						}

						// Fill out the remaining progress bar if later parts have completed.
						if ( ( IS_STATUS_NOT( di->status, STATUS_CONNECTING ) || range_info_count >= total_range_info_count ) && /*last_host_end*/ last_range_end < di->file_size )
						{
							int range_offset = 0;
							if ( di->file_size > 0 )
							{
#ifdef _WIN64
								range_offset = ( int )_ceil( ( double )progress_width * ( ( double )last_range_end / ( double )di->file_size ) );
#else
								double f_range_offset = _ceil( ( double )progress_width * ( ( double )last_range_end / ( double )di->file_size ) );
								__asm
								{
									fld f_range_offset;	//; Load the floating point value onto the FPU stack.
									fistp range_offset;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}
#endif
							}

							_BitBlt( hdcMem, progress_rc.left + range_offset, progress_rc.top, progress_width - range_offset, progress_height, hdcMem2, range_offset, 0, SRCCOPY );
						}
					}
					else
					{
						int progress_offset;

						// Connecting, Downloading, Paused, Queued, Stopped.
						if ( IS_STATUS( di->status,
								STATUS_CONNECTING |
								STATUS_DOWNLOADING |
								STATUS_STOPPED |
								STATUS_QUEUED ) ||
								di->status == STATUS_NONE )
						{
							if ( di->file_size > 0 )
							{
								int i_percentage;
#ifdef _WIN64
								i_percentage = ( int )_ceil( ( double )progress_width * ( ( double )di->last_downloaded / ( double )di->file_size ) );
#else
								// Multiply the floating point division by 100%.
								double f_percentage = _ceil( ( double )progress_width * ( ( double )di->last_downloaded / ( double )di->file_size ) );
								__asm
								{
									fld f_percentage;	//; Load the floating point value onto the FPU stack.
									fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}
#endif
								//rc_clip.right = i_percentage;
								progress_offset = i_percentage;
							}
							else
							{
								//rc_clip.right = 0;
								progress_offset = 0;
							}
						}
						else
						{
							progress_offset = progress_width;
						}

						RECT progress_parts_rc;
						progress_parts_rc.top = 0;
						progress_parts_rc.left = 0;
						progress_parts_rc.right = progress_width;
						progress_parts_rc.bottom = progress_height;

						// We'll pick out sections of this bitmap to paint the progress bar.
						color = _CreateSolidBrush( color_ref_body );
						_FillRect( hdcMem2, &progress_parts_rc, color );
						_DeleteObject( color );

						RECT parts_text_rc;
						parts_text_rc.left = 0;
						parts_text_rc.right = progress_rc.right - progress_rc.left;
						parts_text_rc.top = 0;
						parts_text_rc.bottom = progress_rc.bottom - progress_rc.top;

						_SetTextColor( hdcMem2, color_ref_body_text );
						__DrawTextW( hdcMem2, 0, 0, align | _DT_VCENTER, &parts_text_rc, buf, buf_length );

						_BitBlt( hdcMem, progress_rc.left, progress_rc.top, progress_offset, progress_height, hdcMem2, 0, 0, SRCCOPY );
					}

					_DeleteObject( hbm );
					_DeleteDC( hdcMem2 );

					////////////////////

					RECT frame_rc;
					frame_rc.top = item_rc.top + _SCALE_TLV_( 1 ) + gridline_offset;
					frame_rc.bottom = item_rc.bottom - _SCALE_TLV_( 1 );
					frame_rc.left = item_rc.left + _SCALE_TLV_( 2 );
					if ( i == 0 )
					{
						frame_rc.left += ( g_glyph_offset + g_glyph_size.cx );
					}
					frame_rc.right = item_rc.right - _SCALE_TLV_( 1 );
					if ( frame_rc.right < frame_rc.left )
					{
						frame_rc.right = frame_rc.left;
					}

					HPEN hPen = _CreatePen( PS_SOLID, _SCALE_TLV_( 1 ), color_ref_border );
					HPEN old_color = ( HPEN )_SelectObject( hdcMem, hPen );
					_DeleteObject( old_color );
					HBRUSH old_brush = ( HBRUSH )_SelectObject( hdcMem, _GetStockObject( NULL_BRUSH ) );
					_DeleteObject( old_brush );
					_Rectangle( hdcMem, frame_rc.left, frame_rc.top, frame_rc.right, frame_rc.bottom );
					_DeleteObject( hPen );
				}
				else
				{
					if ( tln->data_type & TLVDT_GROUP ||
					   ( arr2[ i ] != COLUMN_DATE_AND_TIME_ADDED &&
						 arr2[ i ] != COLUMN_CATEGORY &&
						 arr2[ i ] != COLUMN_DOWNLOAD_DIRECTORY &&
						 arr2[ i ] != COLUMN_FILENAME ) )
					{
						// Draw selected text
						if ( selected )
						{
							_SetTextColor( hdcMem, ( node_count & 1 ? cfg_even_row_highlight_font_color : cfg_odd_row_highlight_font_color ) );
						}
						else
						{
							_SetTextColor( hdcMem, ( node_count & 1 ? cfg_even_row_font_settings.font_color : cfg_odd_row_font_settings.font_color ) );
						}

						__DrawTextW( hdcMem, text_rc.left, text_rc.top, align | _DT_VCENTER, &text_rc, buf, buf_length );
					}
				}
			}
		}

		++node_count;

		tln = TLV_NextNode( tln, false );

		if ( tln != NULL && tln->parent != NULL )
		{
			++child_count;
		}
		else
		{
			++root_count;

			child_count = 0;
		}

		// Only draw as many rows as can be displayed.
		if ( ++row_count == g_visible_rows )
		{
			break;
		}
	}

	if ( cfg_draw_all_rows )
	{
		while ( row_count < g_visible_rows )
		{
			row_rc.top = row_rc.bottom;
			row_rc.bottom += g_row_height;

			RECT row_bg_rc;
			row_bg_rc.top = row_rc.top + _SCALE_TLV_( 1 );
			row_bg_rc.left = row_rc.left;
			row_bg_rc.right = ( cfg_draw_full_rows ? g_client_rc.right : row_rc.right );
			row_bg_rc.bottom = row_rc.bottom;

			// Draw odd/even rectangle.
			HBRUSH color = _CreateSolidBrush( ( node_count & 1 ? cfg_even_row_background_color : cfg_odd_row_background_color ) );

			_FillRect( hdcMem, &row_bg_rc, color );
			_DeleteObject( color );

			++node_count;

			++row_count;
		}
	}

	if ( cfg_show_gridlines )
	{
		HPEN line_color = NULL;

		if ( start_index != end_index || g_visible_rows > 0 )
		{
			line_color = _CreatePen( PS_SOLID, _SCALE_TLV_( 1 ), cfg_gridline_color );
			HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
			_DeleteObject( old_color );
		}

		// Draw column lines.
		for ( char i = start_index; i <= end_index; ++i )
		{
			_MoveToEx( hdcMem, rc_array[ i ].right, g_client_rc.top, NULL );
			_LineTo( hdcMem, rc_array[ i ].right, g_client_rc.bottom );
		}

		// Draw row lines.
		for ( int i = 0; i < g_visible_rows; ++i )
		{
			_MoveToEx( hdcMem, 0, g_client_rc.top + ( i * g_row_height ), NULL );
			_LineTo( hdcMem, g_client_rc.right, g_client_rc.top + ( i * g_row_height ) );
		}

		if ( line_color != NULL )
		{
			_DeleteObject( line_color );
		}
	}

	//

	// Draw the selection marquee rectangle or insertion line.
	if ( g_is_dragging )
	{
		// Draw the insertion line, but only for root items.
		if ( g_draw_drag &&
		   ( g_focused_node == NULL || g_focused_node->parent == NULL ) )
		{
			int focus_offset = ( g_focused_index >= g_expanded_item_count ? g_expanded_item_count : g_focused_index ) - g_first_visible_index;

			HPEN line_color;

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				line_color = _CreatePen( PS_DOT, _SCALE_TLV_( 1 ), dm_color_list_highlight );
			}
			else
#endif
			{
				line_color = _CreatePen( PS_DOT, _SCALE_TLV_( 1 ), ( COLORREF )_GetSysColor( COLOR_HOTLIGHT ) );
			}

			HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
			_DeleteObject( old_color );
			_MoveToEx( hdcMem, 0, g_client_rc.top + ( focus_offset * g_row_height ), NULL );
			_LineTo( hdcMem, g_client_rc.right, g_client_rc.top + ( focus_offset * g_row_height ) );
			_DeleteObject( line_color );
		}
		else
		{
			RECT drag_rc;

			// Creates a rectangle that's slightly larger (at maximum) than our window.
			if ( g_drag_rc.left < -10 )
			{
				drag_rc.left = -10;
			}
			else
			{
				drag_rc.left = g_drag_rc.left;
			}

			if ( g_drag_rc.right > g_client_rc.right + 10 )
			{
				drag_rc.right = g_client_rc.right + 10;
			}
			else if ( g_drag_rc.right >= drag_rc.left )
			{
				drag_rc.right = g_drag_rc.right;
			}
			else
			{
				drag_rc.right = drag_rc.left;
			}

			if ( g_drag_rc.top < 0 )
			{
				drag_rc.top = 0;
			}
			else
			{
				drag_rc.top = g_drag_rc.top;
			}

			if ( g_drag_rc.bottom > g_client_rc.bottom + g_row_height )
			{
				drag_rc.bottom = g_client_rc.bottom + g_row_height;
			}
			else if ( g_drag_rc.bottom >= drag_rc.top )
			{
				drag_rc.bottom = g_drag_rc.bottom;
			}
			else
			{
				drag_rc.bottom = drag_rc.top;
			}

			int height = _abs( drag_rc.bottom - drag_rc.top );
			int width = _abs( drag_rc.right - drag_rc.left );

			if ( height != 0 && width != 0 )
			{
				HDC hdcMem2 = _CreateCompatibleDC( hDC );
				HBITMAP hbm = _CreateCompatibleBitmap( hDC, 1, 1 );
				ohbm = ( HBITMAP )_SelectObject( hdcMem2, hbm );
				_DeleteObject( ohbm );
				_DeleteObject( hbm );

				RECT body_rc;
				body_rc.left = 0;
				body_rc.top = 0;
				body_rc.right = 1;
				body_rc.bottom = 1;
				background = _CreateSolidBrush( cfg_selection_marquee_color );
				_FillRect( hdcMem2, &body_rc, background );

				// Blend the rectangle into the background to make it look transparent.
				//BLENDFUNCTION blend = { AC_SRC_OVER, 0, 85, AC_SRC_OVER };	// 85 matches Explorer's Detail view.
				//BLENDFUNCTION blend = { AC_SRC_OVER, 0, 70, AC_SRC_OVER };	// 70 matches a ListView control.
				BLENDFUNCTION blend;
				blend.BlendOp = AC_SRC_OVER;
				blend.BlendFlags = 0;
				blend.SourceConstantAlpha = 70;
				blend.AlphaFormat = AC_SRC_OVER;

				_GdiAlphaBlend( hdcMem, drag_rc.left, drag_rc.top, width, height, hdcMem2, 0, 0, 1, 1, blend );
				_DeleteDC( hdcMem2 );

				// Draw a solid border around rectangle.
				//_FrameRect( hdcMem, &drag_rc, background );
				HRGN hRgn = _CreateRectRgn( drag_rc.left, drag_rc.top, drag_rc.right, drag_rc.bottom );
				_FrameRgn( hdcMem, hRgn, background, _SCALE_TLV_( 1 ), _SCALE_TLV_( 1 ) );
				_DeleteObject( hRgn );

				_DeleteObject( background );
			}
		}
	}

	//

	/*if ( !cfg_show_column_headers )
	{
		// Draw the single border line at the top of the window.
		// It will overlap the first item's top line if there's no header visible.
		HPEN line_color = _CreatePen( PS_SOLID, _SCALE_TLV_( 1 ), ( COLORREF )_GetSysColor( COLOR_3DSHADOW ) );
		HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
		_DeleteObject( old_color );

		_MoveToEx( hdcMem, 0, 0, NULL );
		_LineTo( hdcMem, rc.right, 0 );
		_DeleteObject( line_color );
	}*/

	//

	// Draw our memory buffer to the main device context.
	_BitBlt( hDC, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY );

	_DeleteDC( hdcMem );
	_EndPaint( hWnd, &ps );
}

void CreateTooltip( HWND hWnd )
{
	g_tlv_tooltip_buffer = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * 512 );

	g_hWnd_tlv_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, NULL, NULL );

	TOOLINFO tti;
	_memzero( &tti, sizeof( TOOLINFO ) );
	tti.cbSize = sizeof( TOOLINFO );
	tti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	tti.hwnd = hWnd;
	tti.uId = ( UINT_PTR )hWnd;

	_SendMessageW( g_hWnd_tlv_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
	_SendMessageW( g_hWnd_tlv_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );
	_SendMessageW( g_hWnd_tlv_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );
}

void AdjustRowHeight()
{
	int height1, height2;
	TEXTMETRIC tm;
	HDC hDC = _GetDC( NULL );
	HFONT ohf = ( HFONT )_SelectObject( hDC, tlv_odd_row_font_settings.font );
	_GetTextMetricsW( hDC, &tm );
	height1 = tm.tmHeight + tm.tmExternalLeading + _SCALE_TLV_( 5 );
	_SelectObject( hDC, ohf );	// Reset old font.
	ohf = ( HFONT )_SelectObject( hDC, tlv_even_row_font_settings.font );
	_GetTextMetricsW( hDC, &tm );
	height2 = tm.tmHeight + tm.tmExternalLeading + _SCALE_TLV_( 5 );
	_SelectObject( hDC, ohf );	// Reset old font.
	_ReleaseDC( NULL, hDC );

	g_row_height = ( height1 > height2 ? height1 : height2 );

	int icon_height = __GetSystemMetricsForDpi( SM_CYSMICON, current_dpi_tlv ) + _SCALE_TLV_( 2 );
	if ( g_row_height < icon_height )
	{
		g_row_height = icon_height;
	}
}

LRESULT CALLBACK TreeListViewWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			current_dpi_tlv = __GetDpiForWindow( hWnd );
			hFont_tlv = UpdateFont( current_dpi_tlv );

			//tlv_odd_row_font_settings.font = _CreateFontIndirectW( &cfg_odd_row_font_settings.lf );
			tlv_odd_row_font_settings.font_color = cfg_odd_row_font_settings.font_color;
			_memcpy_s( &tlv_odd_row_font_settings.lf, sizeof( LOGFONT ), &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ) );

			//tlv_even_row_font_settings.font = _CreateFontIndirectW( &cfg_even_row_font_settings.lf );
			tlv_even_row_font_settings.font_color = cfg_even_row_font_settings.font_color;
			_memcpy_s( &tlv_even_row_font_settings.lf, sizeof( LOGFONT ), &cfg_even_row_font_settings.lf, sizeof( LOGFONT ) );

			// The font will not have changed here, only its height.
			tlv_odd_row_font_settings.lf.lfHeight = _SCALE_TLV_( cfg_odd_row_font_settings.lf.lfHeight );
			tlv_odd_row_font_settings.font = _CreateFontIndirectW( &tlv_odd_row_font_settings.lf );

			// The font will not have changed here, only its height.
			tlv_even_row_font_settings.lf.lfHeight = _SCALE_TLV_( cfg_even_row_font_settings.lf.lfHeight );
			tlv_even_row_font_settings.font = _CreateFontIndirectW( &tlv_even_row_font_settings.lf );

			g_hWnd_tlv_header = _CreateWindowW( WC_HEADER, NULL, HDS_BUTTONS | HDS_DRAGDROP | HDS_FULLDRAG | HDS_HOTTRACK | HDS_HORZ | WS_CHILDWINDOW, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			CreateTooltip( hWnd );

			AdjustRowHeight();

			int arr[ NUM_COLUMNS ];

			// Initialize our treelistview columns
			HDITEM hdi;
			_memzero( &hdi, sizeof( HDITEM ) );
			hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;

			for ( char i = 0; i < NUM_COLUMNS; ++i )
			{
				// Active Parts, Download Speed, Download Speed Limit, Downloaded, File Size, Time Elapsed, Time Remaining
				if ( i == COLUMN_ACTIVE_PARTS ||
					 i == COLUMN_DOWNLOAD_SPEED ||
					 i == COLUMN_DOWNLOAD_SPEED_LIMIT ||
					 i == COLUMN_DOWNLOADED ||
					 i == COLUMN_FILE_SIZE ||
					 i == COLUMN_TIME_ELAPSED ||
					 i == COLUMN_TIME_REMAINING )
				{
					hdi.fmt = HDF_RIGHT | HDF_STRING;
				}
				else if ( i == COLUMN_PROGRESS )	// Progress
				{
					hdi.fmt = HDF_CENTER | HDF_STRING;
				}
				else
				{
					hdi.fmt = HDF_LEFT | HDF_STRING;
				}

				if ( i != 0 && i == cfg_sorted_column_index )
				{
					hdi.fmt = hdi.fmt | ( cfg_sorted_direction == 1 ? HDF_SORTUP : HDF_SORTDOWN );
				}

				if ( *download_columns[ i ] != -1 )
				{
					hdi.pszText = g_locale_table[ DOWNLOAD_STRING_TABLE_OFFSET + i ].value;
					hdi.cchTextMax = g_locale_table[ DOWNLOAD_STRING_TABLE_OFFSET + i ].length;
					hdi.cxy = _SCALE_TLV_( *download_columns_width[ i ] );

					g_header_width += hdi.cxy;

					_SendMessageW( g_hWnd_tlv_header, HDM_INSERTITEM, g_total_columns, ( LPARAM )&hdi );

					arr[ g_total_columns++ ] = *download_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_tlv_header, HDM_SETORDERARRAY, g_total_columns, ( LPARAM )arr );

			_SendMessageW( g_hWnd_tlv_header, WM_SETFONT, ( WPARAM )hFont_tlv, 0 );

			if ( cfg_show_column_headers )
			{
				WINDOWPOS wp;
				_memzero( &wp, sizeof ( WINDOWPOS ) );
				RECT rc;
				rc.top = rc.left = 0;
				rc.bottom = _SCALE_TLV_( 100 );
				rc.right = g_header_width;
				HDLAYOUT hdl;
				hdl.prc = &rc;
				hdl.pwpos = &wp;
				_SendMessageW( g_hWnd_tlv_header, HDM_LAYOUT, 0, ( LPARAM )&hdl );

				g_header_height = ( wp.cy - wp.y );

				_SetWindowPos( g_hWnd_tlv_header, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW );
			}
			else
			{
				g_header_height = 0;
			}

			bool use_theme = true;
			#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					use_theme = InitializeUXTheme();
				}
			#endif

			if ( use_theme )
			{
				g_hTheme = _OpenThemeData( hWnd, L"TreeView" );
			}
			else
			{
				g_hTheme = NULL;
			}
			
			g_glyph_size.cx = 0;
			g_glyph_size.cy = 0;

			if ( g_hTheme != NULL )
			{
				_GetThemePartSize( g_hTheme, NULL, TVP_GLYPH, GLPS_OPENED, NULL, TS_DRAW, &g_glyph_size );
			}

			return 0;
		}
		break;

		case WM_GETDLGCODE:
		{
			// Don't process the tab key if we're focusing on a window with scrollbars.
			if ( wParam == VK_TAB && !( _GetKeyState( VK_SHIFT ) & 0x8000 ) )
			{
				// returning DLGC_WANTTAB will cause a beep.
				LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

				//_SetFocus( g_hWnd_tlv_files );

				return ret;
			}

			return DLGC_WANTALLKEYS;
		}
		break;

		case TLVM_REFRESH_LIST:
		{
			if ( wParam == TRUE )
			{
				HandleWindowChange( hWnd, ( lParam == TRUE ? true : false ) );
			}

			_InvalidateRect( hWnd, &g_client_rc, TRUE );
		}
		break;

		case TLVM_EDIT_LABEL:
		{
			if ( !in_worker_thread && g_selected_count == 1 && g_base_selected_node != NULL && g_base_selected_node->data_type & TLVDT_GROUP )
			{
				// See if we've clicked within the Filename column so that we can show the edit textbox.
				int index = GetColumnIndexFromVirtualIndex( *download_columns[ COLUMN_FILENAME ], download_columns, NUM_COLUMNS );

				// Allow the edit textbox to be shown if we've clicked within the Filename column.
				if ( index != -1 && *download_columns[ COLUMN_FILENAME ] != -1 )
				{
					_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, index, ( LPARAM )&g_edit_column_rc );

					SCROLLINFO si;
					_memzero( &si, sizeof( SCROLLINFO ) );
					si.cbSize = sizeof( SCROLLINFO );
					si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
					_GetScrollInfo( hWnd, SB_HORZ, &si );

					g_edit_column_rc.left -= si.nPos;
					g_edit_column_rc.right -= si.nPos;

					int offset = 0;
					if ( g_edit_column_rc.left < 0 )
					{
						offset = Scroll( &si, SCROLL_TYPE_LEFT, -g_edit_column_rc.left );

						g_edit_column_rc.left += offset;
						g_edit_column_rc.right += offset;
					}
					else if ( g_edit_column_rc.left > ( LONG )( si.nPos + si.nPage ) )
					{
						offset = Scroll( &si, SCROLL_TYPE_RIGHT, g_edit_column_rc.left );

						g_edit_column_rc.left -= offset;
						g_edit_column_rc.right -= offset;
					}

					if ( offset != 0 )
					{
						HDWP hdwp = _BeginDeferWindowPos( 1 );
						_DeferWindowPos( hdwp, g_hWnd_tlv_header, HWND_TOP, -si.nPos, 0, g_client_rc.right + si.nPos, g_header_height, 0 );
						_EndDeferWindowPos( hdwp );

						_SetScrollInfo( hWnd, SB_HORZ, &si, TRUE );

						_InvalidateRect( hWnd, &g_client_rc, TRUE );
					}
				}

				_KillTimer( hWnd, IDT_EDIT_TIMER );

				g_show_edit_state = 1;

				if ( g_hWnd_edit_box == NULL )
				{
					CreateEditBox( hWnd );
				}
			}
		}
		break;

		case TLVM_CANCEL_EDIT:
		{
			TLV_CancelRename( hWnd );
		}
		break;

		case TLVM_SH_COLUMN_HEADERS:
		{
			if ( wParam == TRUE )
			{
				_ShowWindow( g_hWnd_tlv_header, SW_SHOW );

				WINDOWPOS wp;
				_memzero( &wp, sizeof ( WINDOWPOS ) );
				RECT rc;
				rc.top = rc.left = 0;
				rc.bottom = 100;
				rc.right = g_header_width;
				HDLAYOUT hdl;
				hdl.prc = &rc;
				hdl.pwpos = &wp;
				_SendMessageW( g_hWnd_tlv_header, HDM_LAYOUT, 0, ( LPARAM )&hdl );

				g_header_height = ( wp.cy - wp.y );
			}
			else
			{
				_ShowWindow( g_hWnd_tlv_header, SW_HIDE );

				g_header_height = 0;
			}

			g_size_changed = true;

			HandleWindowChange( hWnd );

			_InvalidateRect( hWnd, NULL, TRUE );
		}
		break;

		case TLVM_TOGGLE_DRAW:
		{
			g_skip_list_draw = ( wParam == TRUE ? true : false );
		}
		break;

		case TLVM_SORT_ITEMS:
		{
			TLV_MergeSort( &g_tree_list, g_total_parent_item_nodes/*g_root_item_count*/, &merge_compare, ( void * )lParam );

			TLV_CleanupSort();
		}
		break;

		case TLVM_CANCEL_DRAG:
		{
			if ( g_draw_drag )
			{
				TLV_ClearDrag();
			}
		}
		break;

		case TLVM_CANCEL_SELECT:
		{
			TLV_ClearDrag();
		}
		break;

		case TLVM_UPDATE_FONTS:
		{
			_memcpy_s( &tlv_odd_row_font_settings.lf, sizeof( LOGFONT ), &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ) );
			tlv_odd_row_font_settings.lf.lfHeight = _SCALE_TLV_( tlv_odd_row_font_settings.lf.lfHeight );
			_DeleteObject( tlv_odd_row_font_settings.font );
			tlv_odd_row_font_settings.font = _CreateFontIndirectW( &tlv_odd_row_font_settings.lf );

			_memcpy_s( &tlv_even_row_font_settings.lf, sizeof( LOGFONT ), &cfg_even_row_font_settings.lf, sizeof( LOGFONT ) );
			tlv_even_row_font_settings.lf.lfHeight = _SCALE_TLV_( tlv_even_row_font_settings.lf.lfHeight );
			_DeleteObject( tlv_even_row_font_settings.font );
			tlv_even_row_font_settings.font = _CreateFontIndirectW( &tlv_even_row_font_settings.lf );

			AdjustRowHeight();
		}
		break;

		case WM_HSCROLL:
		case WM_VSCROLL:
		{
			_SetFocus( hWnd );

			TLV_Scroll( hWnd, ( msg == WM_HSCROLL ? 1 : 0 ), LOWORD( wParam ) );
		}
		break;

		case WM_CHAR:
		{
			if ( ( _GetKeyState( VK_CONTROL ) & 0x8000 ) || g_tree_list == NULL )
			{
				break;
			}

			DWORD current_typing_time = GetTickCount();
			DWORD delta = current_typing_time - g_last_typing_time;
			g_last_typing_time = current_typing_time;

			wchar_t wc = ( wchar_t )wParam;

			/*if ( ( wc >= 0x30 && wc <= 0x39 ) ||
				 ( wc >= 0x41 && wc <= 0x5A ) ||
				 ( wc >= 0x61 && wc <= 0x7A ) ||
					wc == L'.' || wc == L'`' || wc == L'!' ||
					wc == L'@' || wc == L'#' || wc == L'$' ||
					wc == L'%' || wc == L'^' || wc == L'&' ||
					wc == L'*' || wc == L'(' || wc == L')' ||
					wc == L'-' || wc == L'_' || wc == L'+' ||
					wc == L'=' || wc == L'\\'|| wc == L']' ||
					wc == L'}' || wc == L'[' || wc == L'{' ||
					wc == L'/' || wc == L'?' || wc == L'>' ||
					wc == L'<' || wc == L',' || wc == L'~' )
			{}*/

			TREELISTNODE *last_sel_tln = NULL;

			TREELISTNODE *tln = TLV_GetFocusedItem();
			int current_item_index = TLV_GetFocusedIndex();

			if ( delta >= 0 && delta < TYPING_DELAY )
			{
				if ( g_typing_buf_offset < ( MAX_PATH - 1 ) )
				{
					g_typing_buf[ g_typing_buf_offset++ ] = wc;
				}
			}
			else
			{
				g_typing_buf_offset = 1;
				g_typing_buf[ 0 ] = wc;

				if ( tln != NULL )
				{
					if ( tln->parent != NULL )
					{
						// Include the parent.
						current_item_index = TLV_GetParentIndex( tln, current_item_index ) + 1;

						if ( tln->parent->is_expanded )
						{
							current_item_index += tln->parent->child_count;
						}

						tln = tln->parent->next;
					}
					else
					{
						++current_item_index;

						if ( tln->is_expanded )
						{
							current_item_index += tln->child_count;
						}

						tln = tln->next;
					}
				}
			}

			g_typing_buf[ g_typing_buf_offset ] = 0;

			if ( tln == NULL )
			{
				tln = g_tree_list;
				current_item_index = 0;
			}

			TREELISTNODE *start_tln = tln;

			TLV_ClearSelected( false, false );
			TLV_ResetSelectionBounds();

			int sel_count = 0;

			do
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					// Only adjust the node values (indices and nodes) if we're on the current filter.
					if ( g_status_filter == STATUS_NONE || IsFilterSet( di, g_status_filter ) )
					{
						if ( _StrCmpNIW( di->file_path + di->filename_offset, g_typing_buf, g_typing_buf_offset ) == 0 )
						{
							++sel_count;

							TLV_SetSelectionBounds( current_item_index, tln );

							TLV_SetFocusedItem( tln );
							TLV_SetFocusedIndex( current_item_index );

							if ( last_sel_tln != NULL )
							{
								last_sel_tln->flag = TLVS_SELECTED;
							}

							tln->flag = TLVS_SELECTED | TLVS_FOCUSED;

							last_sel_tln = tln;

							int visible_item_count = TLV_GetVisibleItemCount();
							int first_visible_index = TLV_GetFirstVisibleIndex();

							if ( current_item_index >= first_visible_index + visible_item_count )
							{
								TREELISTNODE *first_visible_node = tln;

								for ( ; visible_item_count > 1 && first_visible_node != g_tree_list; --current_item_index, --visible_item_count )
								{
									first_visible_node = TLV_PrevNode( first_visible_node, false );
								}

								int root_index = TLV_GetFirstVisibleRootIndex();

								TREELISTNODE *current_first_visible_parent_node = TLV_GetFirstVisibleItem();
								current_first_visible_parent_node = ( current_first_visible_parent_node != NULL && current_first_visible_parent_node->parent != NULL ? current_first_visible_parent_node->parent : current_first_visible_parent_node );
								TREELISTNODE *first_visible_parent_node = ( first_visible_node != NULL && first_visible_node->parent != NULL ? first_visible_node->parent : first_visible_node );
								while ( current_first_visible_parent_node != NULL && current_first_visible_parent_node != first_visible_parent_node )
								{
									di = ( DOWNLOAD_INFO * )current_first_visible_parent_node->data;

									current_first_visible_parent_node = current_first_visible_parent_node->next;

									// This shouldn't be true since we're filtering as we're iterating across the entire list.
									if ( g_status_filter != STATUS_NONE && !IsFilterSet( di, g_status_filter ) )
									{
										continue;
									}

									++root_index;
								}

								TLV_SetFirstVisibleRootIndex( root_index );
								TLV_SetFirstVisibleItem( first_visible_node );
								TLV_SetFirstVisibleIndex( current_item_index );
							}
							else if ( current_item_index < first_visible_index )
							{
								int root_index = TLV_GetFirstVisibleRootIndex();

								TREELISTNODE *current_first_visible_parent_node = TLV_GetFirstVisibleItem();
								current_first_visible_parent_node = ( current_first_visible_parent_node != NULL && current_first_visible_parent_node->parent != NULL ? current_first_visible_parent_node->parent : current_first_visible_parent_node );
								TREELISTNODE *first_visible_parent_node = ( tln != NULL && tln->parent != NULL ? tln->parent : tln );
								while ( current_first_visible_parent_node != NULL && current_first_visible_parent_node != first_visible_parent_node )
								{
									di = ( DOWNLOAD_INFO * )current_first_visible_parent_node->data;

									current_first_visible_parent_node = current_first_visible_parent_node->prev;

									if ( g_status_filter != STATUS_NONE && !IsFilterSet( di, g_status_filter ) )
									{
										continue;
									}

									--root_index;
								}

								TLV_SetFirstVisibleRootIndex( root_index );

								TLV_SetFirstVisibleItem( tln );
								TLV_SetFirstVisibleIndex( current_item_index );
							}

							break;
						}

						++current_item_index;

						if ( tln->is_expanded )
						{
							current_item_index += tln->child_count;
						}
					}
				}

				// Search root nodes.
				tln = tln->next;

				if ( tln == NULL )
				{
					tln = g_tree_list;
					current_item_index = 0;
				}
			}
			while ( tln != start_tln );

			TLV_SetSelectedCount( sel_count );

			HandleWindowChange( hWnd );

			_InvalidateRect( hWnd, &g_client_rc, TRUE );
		}
		break;

		case WM_KEYDOWN:
		{
			// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
			if ( _GetKeyState( VK_CONTROL ) & 0x8000 && wParam != VK_UP && wParam != VK_DOWN )
			{
				switch ( wParam )
				{
					case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
					{
						if ( !in_worker_thread && g_expanded_item_count > 0 )
						{
							HandleCommand( hWnd, MENU_SELECT_ALL );
						}
					}
					break;

					case 'C':	// Copy URL(s).
					{
						if ( !in_worker_thread && g_selected_count > 0 )
						{
							HandleCommand( hWnd, MENU_COPY_URLS );
						}
					}
					break;

					case 'N':	// Open Add URL(s) window.
					{
						HandleCommand( hWnd, MENU_ADD_URLS );
					}
					break;

					case 'O':	// Open Options window.
					{
						HandleCommand( hWnd, MENU_OPTIONS );
					}
					break;

					case 'R':	// Remove selected items.
					{
						if ( !in_worker_thread && g_selected_count > 0 )
						{
							HandleCommand( hWnd, MENU_REMOVE );
						}
					}
					break;

					case 'S':	// Open Search window.
					{
						HandleCommand( hWnd, MENU_SEARCH );
					}
					break;

					case 'L':	// Open Global Download Speed Limit window.
					{
						HandleCommand( hWnd, MENU_GLOBAL_SPEED_LIMIT );
					}
					break;

					case 'M':
					{
						HandleCommand( hWnd, MENU_SITE_MANAGER );
					}
					break;

					case 'U':
					{
						HandleCommand( hWnd, MENU_UPDATE_DOWNLOAD );
					}
					break;

					case VK_DELETE:	// Remove and Delete selected items.
					{
						if ( !in_worker_thread && g_selected_count > 0 )
						{
							HandleCommand( hWnd, MENU_REMOVE_AND_DELETE );
						}
					}
					break;
				}

				if ( _GetKeyState( VK_SHIFT ) & 0x8000 )
				{
					if ( wParam == 'E' )
					{
						if ( !in_worker_thread )
						{
							HandleCommand( hWnd, MENU_LIST_EDIT_MODE );
						}
					}
					else if ( wParam == 'R' )
					{
						if ( !in_worker_thread )
						{
							HandleCommand( hWnd, MENU_REMOVE_COMPLETED );
						}
					}
				}
			}
			else
			{
				switch ( wParam )
				{
					case VK_PRIOR:
					{
						if ( g_tree_list != NULL )
						{
							TLV_ClearDrag();
							TLV_ClearSelected( false, false );

							TREELISTNODE *focused_node;
							int focused_index;

							int offset;

							if ( g_focused_index < 0 || g_focused_index > g_first_visible_index )
							{
								offset = 0;

								focused_node = TLV_GetFirstVisibleItem();
								focused_index = g_first_visible_index;
							}
							else
							{
								focused_node = g_focused_node;
								focused_index = g_focused_index;

								TREELISTNODE *first_node = g_tree_list;
								if ( g_status_filter != STATUS_NONE )
								{
									DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )first_node->data;
									if ( !IsFilterSet( di, g_status_filter ) )
									{
										first_node = TLV_NextNode( first_node, false );
										if ( first_node == NULL )
										{
											break;
										}
									}
								}

								int visible_item_count = max( ( g_visible_item_count - 1 ), 1 );

								while ( focused_node != first_node && visible_item_count > 0 )
								{
									focused_node = TLV_PrevNode( focused_node, false );
									--focused_index;

									--visible_item_count;
								}

								offset = g_first_visible_index - focused_index;
							}

							if ( focused_node != NULL )
							{
								/*if ( focused_node->parent != NULL )
								{
									focused_index = TLV_GetParentIndex( focused_node, focused_index );
									focused_node = focused_node->parent;
								}*/

								focused_node->flag |= ( TLVS_SELECTED | TLVS_FOCUSED );

								g_selected_count = 1;
							}
							else
							{
								focused_index = -1;
								g_selected_count = 0;
							}

							g_focused_node = g_first_selection_node = g_last_selection_node = g_base_selected_node = focused_node;
							g_focused_index = g_first_selection_index = g_last_selection_index = g_base_selected_index = focused_index;

							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							offset = Scroll( &si, SCROLL_TYPE_UP, offset );

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}

							_InvalidateRect( hWnd, &g_client_rc, TRUE );

							if ( !in_worker_thread )
							{
								UpdateMenus( true );
							}
						}

						//TLV_Scroll( hWnd, 0, SB_PAGEUP );
					}
					break;

					case VK_NEXT:
					{
						if ( g_tree_list != NULL )
						{
							TLV_ClearDrag();
							TLV_ClearSelected( false, false );

							TREELISTNODE *focused_node;
							int focused_index;

							int offset;

							if ( g_visible_item_count <= 1 )
							{
								offset = 1;

								focused_node = TLV_NextNode( g_first_visible_node, false );
								if ( focused_node == NULL )
								{
									focused_node = TLV_GetFirstVisibleItem();
									focused_index = g_first_visible_index;
								}
								else
								{
									focused_index = g_first_visible_index + 1;
								}
							}
							else if ( g_focused_index >= g_expanded_item_count || g_focused_index < ( g_visible_item_count - 1 ) || g_focused_index < g_first_visible_index )
							{
								offset = 0;

								focused_node = TLV_GetFirstVisibleItem();
								focused_index = g_first_visible_index;
							}
							else
							{
								offset = g_focused_index - g_first_visible_index;

								focused_node = g_focused_node;
								focused_index = g_focused_index;
							}

							TREELISTNODE *tmp_focused_node = focused_node;
							for ( int i = 0; i < ( g_visible_item_count - 1 ); ++i )
							{
								tmp_focused_node = TLV_NextNode( tmp_focused_node, false );
								if ( tmp_focused_node == NULL )
								{
									break;
								}
								else
								{
									focused_node = tmp_focused_node;
									++focused_index;
								}
							}

							if ( focused_node != NULL )
							{
								/*if ( focused_node->parent != NULL )
								{
									focused_index = TLV_GetParentIndex( focused_node, focused_index );
									focused_node = focused_node->parent;
								}*/

								focused_node->flag |= ( TLVS_SELECTED | TLVS_FOCUSED );

								g_selected_count = 1;
							}
							else
							{
								focused_index = -1;
								g_selected_count = 0;
							}

							g_focused_node = g_first_selection_node = g_last_selection_node = g_base_selected_node = focused_node;
							g_focused_index = g_first_selection_index = g_last_selection_index = g_base_selected_index = focused_index;

							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							offset = Scroll( &si, SCROLL_TYPE_DOWN, offset );

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}

							_InvalidateRect( hWnd, &g_client_rc, TRUE );

							if ( !in_worker_thread )
							{
								UpdateMenus( true );
							}
						}

						//TLV_Scroll( hWnd, 0, SB_PAGEDOWN );
					}
					break;

					case VK_END:
					{
						if ( g_tree_list != NULL )
						{
							TLV_ClearDrag();
							TLV_ClearSelected( false, false );

							TREELISTNODE *focused_node;
							int focused_index;

							if ( g_tree_list->prev != NULL )
							{
								focused_node = g_tree_list->prev;
							}
							else
							{
								focused_node = g_tree_list;
							}

							if ( g_status_filter != STATUS_NONE )
							{
								while ( focused_node != NULL )
								{
									DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )focused_node->data;
									if ( IsFilterSet( di, g_status_filter ) )
									{
										break;
									}

									if ( focused_node == g_tree_list )
									{
										focused_node = NULL;
									}
									else
									{
										focused_node = TLV_PrevNode( focused_node, false );
									}
								}
							}

							if ( focused_node != NULL )
							{
								if ( focused_node->is_expanded && focused_node->child != NULL )
								{
									if ( focused_node->child->prev != NULL )
									{
										focused_node = focused_node->child->prev;
									}
									else
									{
										focused_node = focused_node->child;
									}
								}

								/*if ( focused_node->parent != NULL )
								{
									focused_index = TLV_GetParentIndex( focused_node, focused_index );
									focused_node = focused_node->parent;
								}*/

								focused_node->flag |= ( TLVS_SELECTED | TLVS_FOCUSED );

								focused_index = g_expanded_item_count - 1;
								g_selected_count = 1;
							}
							else
							{
								focused_index = -1;
								g_selected_count = 0;
							}

							g_focused_node = g_first_selection_node = g_last_selection_node = g_base_selected_node = focused_node;
							g_focused_index = g_first_selection_index = g_last_selection_index = g_base_selected_index = focused_index;

							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							int offset = Scroll( &si, SCROLL_TYPE_DOWN, si.nMax );

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}

							_InvalidateRect( hWnd, &g_client_rc, TRUE );

							if ( !in_worker_thread )
							{
								UpdateMenus( true );
							}
						}

						//TLV_Scroll( hWnd, 0, SB_BOTTOM );
					}
					break;

					case VK_HOME:
					{
						if ( g_tree_list != NULL )
						{
							TLV_ClearDrag();
							TLV_ClearSelected( false, false );

							TREELISTNODE *focused_node = g_tree_list;
							int focused_index = 0;

							if ( g_status_filter != STATUS_NONE )
							{
								DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )focused_node->data;
								if ( !IsFilterSet( di, g_status_filter ) )
								{
									focused_node = TLV_NextNode( focused_node, false );
								}
							}

							if ( focused_node != NULL )
							{
								focused_node->flag |= ( TLVS_SELECTED | TLVS_FOCUSED );

								g_selected_count = 1;
							}
							else
							{
								focused_index = -1;
								g_selected_count = 0;
							}

							g_focused_node = g_first_selection_node = g_last_selection_node = g_base_selected_node = focused_node;
							g_focused_index = g_first_selection_index = g_last_selection_index = g_base_selected_index = focused_index;

							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							int offset = Scroll( &si, SCROLL_TYPE_UP, si.nMax );

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}

							_InvalidateRect( hWnd, &g_client_rc, TRUE );

							if ( !in_worker_thread )
							{
								UpdateMenus( true );
							}
						}

						//TLV_Scroll( hWnd, 0, SB_TOP );
					}
					break;

					case VK_LEFT:
					case VK_RIGHT:
					{
						TLV_ClearDrag();

						if ( g_focused_node != NULL )
						{
							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_POS;
							_GetScrollInfo( hWnd, SB_HORZ, &si );

							if ( wParam == VK_LEFT )
							{
								if ( si.nPos == 0 && g_focused_node->child_count > 0 && g_focused_node->is_expanded )
								{
									TLV_ExpandCollapseParent( g_focused_node, g_focused_index, false );
								}
								else
								{
									TLV_Scroll( hWnd, 1, SB_LINELEFT );
								}
							}
							else// if ( wParam == VK_RIGHT )
							{
								if ( si.nPos == 0 && g_focused_node->child_count > 0 && !g_focused_node->is_expanded )
								{
									TLV_ExpandCollapseParent( g_focused_node, g_focused_index, true );
								}
								else
								{
									TLV_Scroll( hWnd, 1, SB_LINERIGHT );
								}
							}

							HandleWindowChange( hWnd );

							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							int offset = 0;

							if ( g_focused_index < g_first_visible_index )
							{
								offset = Scroll( &si, SCROLL_TYPE_UP, g_first_visible_index - g_focused_index );
							}
							else if ( g_focused_index >= ( g_first_visible_index + g_visible_item_count ) )
							{
								offset = Scroll( &si, SCROLL_TYPE_DOWN, ( g_focused_index + 1 ) - ( g_first_visible_index + g_visible_item_count ) );
							}

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}

							_InvalidateRect( hWnd, &g_client_rc, TRUE );
						}
						else
						{
							if ( wParam == VK_LEFT )
							{
								TLV_Scroll( hWnd, 1, SB_LINELEFT );
							}
							else if ( wParam == VK_RIGHT )
							{
								TLV_Scroll( hWnd, 1, SB_LINERIGHT );
							}
						}
					}
					break;

					case VK_UP:
					case VK_DOWN:
					{
						TLV_ClearDrag();

						// If the base selected node is NULL, then so is the focused node. The opposite is not necessarily true.
						if ( g_base_selected_node == NULL )
						{
							TLV_ClearSelected( false, false );

							g_focused_node = g_base_selected_node = TLV_GetFirstVisibleItem();
							g_focused_index = g_base_selected_index = g_first_visible_index;

							if ( g_focused_node != NULL )
							{
								g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;

								g_selected_count = 1;
							}

							g_first_selection_node = g_last_selection_node = g_focused_node;
							g_first_selection_index = g_last_selection_index = g_focused_index;
						}
						else
						{
							TREELISTNODE *old_focused_node = g_focused_node;
							int old_focused_index = g_focused_index;

							bool ctrl_down = ( _GetKeyState( VK_CONTROL ) & 0x8000 ) ? true : false;
							bool shift_down = ( _GetKeyState( VK_SHIFT ) & 0x8000 ) && !ctrl_down ? true : false;

							bool update_selection_bounds = true;

							if ( wParam == VK_DOWN )
							{
								if ( g_focused_node != NULL )
								{
									TREELISTNODE *focused_node = TLV_NextNode( g_focused_node, false );

									if ( focused_node != NULL )
									{
										g_focused_node->flag &= ~TLVS_FOCUSED;

										if ( ctrl_down )
										{
											if ( g_focused_index < g_base_selected_index )
											{
												if ( g_focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }
												else { ++g_selected_count; }

												g_focused_node->flag ^= TLVS_SELECTED;
											}
											else
											{
												if ( focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }
												else { ++g_selected_count; }

												focused_node->flag ^= TLVS_SELECTED;
											}
										}
										else if ( shift_down )
										{
											if ( g_focused_index < g_base_selected_index )
											{
												if ( g_focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }

												g_focused_node->flag = TLVS_NONE;
											}
											else
											{
												if ( !( focused_node->flag & TLVS_SELECTED ) ) { ++g_selected_count; }

												focused_node->flag |= TLVS_SELECTED;
											}
										}
										else
										{
											TLV_ClearSelected( false, false );

											focused_node->flag |= TLVS_SELECTED;

											g_base_selected_node = focused_node;
											g_base_selected_index = g_focused_index + 1;

											g_selected_count = 1;

											g_first_selection_node = g_last_selection_node = g_base_selected_node;
											g_first_selection_index = g_last_selection_index = g_base_selected_index;
										}

										focused_node->flag |= TLVS_FOCUSED;

										g_focused_node = focused_node;
										++g_focused_index;
									}
									else if ( !ctrl_down && !shift_down )	// We've reached the end of the list.
									{
										TLV_ClearSelected( false, false );

										g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;

										g_selected_count = 1;

										g_first_selection_node = g_last_selection_node = g_base_selected_node = g_focused_node;
										g_first_selection_index = g_last_selection_index = g_base_selected_index = g_focused_index;
									}
								}
								else	// No focus node was selected.
								{
									g_focused_node = g_base_selected_node = g_first_selection_node;
									g_focused_index = g_base_selected_index = g_first_selection_index;

									if ( !ctrl_down && !shift_down )
									{
										TLV_ClearSelected( false, false );

										g_selected_count = 1;
									}

									if ( g_focused_node != NULL )
									{
										g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;
									}

									update_selection_bounds = false;
								}
							}
							else// if ( wParam == VK_UP )
							{
								if ( g_focused_node != g_tree_list )
								{
									TREELISTNODE *focused_node = TLV_PrevNode( g_focused_node, false );

									bool valid_node = true;
									if ( g_status_filter != STATUS_NONE && focused_node == g_tree_list )
									{
										DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )focused_node->data;
										if ( !IsFilterSet( di, g_status_filter ) )
										{
											valid_node = false;
										}
									}

									if ( valid_node )
									{
										if ( focused_node != NULL )
										{
											g_focused_node->flag &= ~TLVS_FOCUSED;

											if ( ctrl_down )
											{
												if ( g_focused_index > g_base_selected_index )
												{
													if ( g_focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }
													else { ++g_selected_count; }

													g_focused_node->flag ^= TLVS_SELECTED;
												}
												else
												{
													if ( focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }
													else { ++g_selected_count; }

													focused_node->flag ^= TLVS_SELECTED;
												}
											}
											else if ( shift_down )
											{
												if ( g_focused_index > g_base_selected_index )
												{
													if ( g_focused_node->flag & TLVS_SELECTED ) { --g_selected_count; }

													g_focused_node->flag = TLVS_NONE;
												}
												else
												{
													if ( !( focused_node->flag & TLVS_SELECTED ) ) { ++g_selected_count; }

													focused_node->flag |= TLVS_SELECTED;
												}
											}
											else
											{
												TLV_ClearSelected( false, false );

												focused_node->flag |= TLVS_SELECTED;

												g_base_selected_node = focused_node;
												g_base_selected_index = g_focused_index - 1;

												g_selected_count = 1;

												g_first_selection_node = g_last_selection_node = g_base_selected_node;
												g_first_selection_index = g_last_selection_index = g_base_selected_index;
											}

											focused_node->flag |= TLVS_FOCUSED;

											g_focused_node = focused_node;
											--g_focused_index;
										}
										else	// No focus node was selected.
										{
											g_focused_node = g_base_selected_node = g_first_selection_node;
											g_focused_index = g_base_selected_index = g_first_selection_index;

											if ( !ctrl_down && !shift_down )
											{
												TLV_ClearSelected( false, false );

												g_selected_count = 1;
											}

											if ( g_focused_node != NULL )
											{
												g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;
											}

											update_selection_bounds = false;
										}
									}
									else if ( !ctrl_down && !shift_down )	// We've reached the beginning of the list.
									{
										TLV_ClearSelected( false, false );

										g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;

										g_selected_count = 1;

										g_first_selection_node = g_last_selection_node = g_base_selected_node = g_focused_node;
										g_first_selection_index = g_last_selection_index = g_base_selected_index = g_focused_index;
									}
								}
								else if ( !ctrl_down && !shift_down )	// We've reached the beginning of the list.
								{
									TLV_ClearSelected( false, false );

									g_focused_node->flag = TLVS_SELECTED | TLVS_FOCUSED;

									g_selected_count = 1;

									g_first_selection_node = g_last_selection_node = g_base_selected_node = g_focused_node;
									g_first_selection_index = g_last_selection_index = g_base_selected_index = g_focused_index;
								}
							}

							// Update our selection bounds.
							if ( update_selection_bounds )
							{
								if ( g_focused_index > old_focused_index )
								{
									if ( g_focused_index > g_last_selection_index )
									{
										if ( g_focused_node->flag & TLVS_SELECTED )
										{
											g_last_selection_index = g_focused_index;
											g_last_selection_node = g_focused_node;
										}
									}
									else if ( old_focused_index <= g_first_selection_index )
									{
										if ( !( old_focused_node->flag & TLVS_SELECTED ) )
										{
											g_first_selection_index = g_focused_index;
											g_first_selection_node = g_focused_node;
										}
									}
								}
								else if ( g_focused_index < old_focused_index )
								{
									if ( g_focused_index < g_first_selection_index )
									{
										if ( g_focused_node->flag & TLVS_SELECTED )
										{
											g_first_selection_index = g_focused_index;
											g_first_selection_node = g_focused_node;
										}
									}
									else if ( old_focused_index >= g_last_selection_index )
									{
										if ( !( old_focused_node->flag & TLVS_SELECTED ) )
										{
											g_last_selection_index = g_focused_index;
											g_last_selection_node = g_focused_node;
										}
									}
								}
							}

							SCROLLINFO si;
							_memzero( &si, sizeof( SCROLLINFO ) );
							si.cbSize = sizeof( SCROLLINFO );
							si.fMask = SIF_ALL;
							_GetScrollInfo( hWnd, SB_VERT, &si );

							int offset = 0;

							if ( g_focused_index < g_first_visible_index )
							{
								offset = Scroll( &si, SCROLL_TYPE_UP, g_first_visible_index - g_focused_index );
							}
							else if ( g_focused_index >= ( g_first_visible_index + g_visible_item_count ) )
							{
								offset = Scroll( &si, SCROLL_TYPE_DOWN, ( g_focused_index + 1 ) - ( g_first_visible_index + g_visible_item_count ) );
							}

							if ( offset != 0 )
							{
								_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
							}
						}

						_InvalidateRect( hWnd, &g_client_rc, TRUE );

						if ( !in_worker_thread )
						{
							UpdateMenus( true );
						}

						return 0;
					}
					break;

					case VK_DELETE:	// Delete selected items.
					{
						if ( !in_worker_thread && g_selected_count > 0 )
						{
							HandleCommand( hWnd, MENU_DELETE );
						}
					}
					break;

					case VK_F2:	// Rename selected item.
					{
						_SendMessageW( hWnd, TLVM_EDIT_LABEL, 0, 0 );
					}
					break;

					case VK_APPS:	// Context menu key.
					{
						if ( !in_worker_thread )
						{
							UpdateMenus( true );
						}

						POINT pt;
						pt.x = g_row_height / 2;
						pt.y = g_client_rc.top + ( g_row_height / 2 );
						if ( g_focused_index >= g_first_visible_index && g_focused_index < ( g_first_visible_index + g_visible_item_count ) )
						{
							pt.y += ( ( g_focused_index - g_first_visible_index ) * g_row_height );
						}
						_ClientToScreen( hWnd, &pt );

						_TrackPopupMenu( g_hMenuSub_download, 0, pt.x, pt.y, 0, hWnd, NULL );
					}
					break;
				}
			}
		}
		break;

		case WM_MOUSEWHEEL:
		{
			_SetFocus( hWnd );

			TLV_ClearDrag();

			SCROLLINFO si;
			_memzero( &si, sizeof( SCROLLINFO ) );
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			short scroll = ( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA );

			int scroll_amount = ( ( _GetKeyState( VK_CONTROL ) & 0x8000 ) ? 1 : SCROLL_AMOUNT );

			int offset = 0;

			if ( scroll > 0 )	// Up
			{
				offset = Scroll( &si, SCROLL_TYPE_UP, scroll_amount );
			}
			else if ( scroll < 0 )	// Down
			{
				offset = Scroll( &si, SCROLL_TYPE_DOWN, scroll_amount );
			}

			if ( offset != 0 )
			{
				_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
			}

			_InvalidateRect( hWnd, &g_client_rc, TRUE );

			return 0;
		}
		break;

		case WM_MOUSEHOVER:
		{
			if ( g_tlv_is_tracking == TRUE && !g_is_dragging )
			{
				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;

				POINT cur_pos;
				_GetCursorPos( &cur_pos );
				_ScreenToClient( hWnd, &cur_pos );

				int pick_index;
				TREELISTNODE *tli_node;

				if ( cur_pos.y >= g_client_rc.top )
				{
					pick_index = ( cur_pos.y - g_client_rc.top ) / g_row_height;
					pick_index += g_first_visible_index;

					if ( pick_index < g_expanded_item_count )
					{
						if ( pick_index != g_tlv_last_tooltip_item && !in_worker_thread )
						{
							// Save the last item that was hovered so we don't have to keep calling everything below.
							g_tlv_last_tooltip_item = pick_index;

							// From the first visible node, get the node that was hovered.
							tli_node = TLV_GetFirstVisibleItem();

							for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
							{
								tli_node = TLV_NextNode( tli_node, false );
							}

							if ( tli_node != NULL && tli_node->data != NULL )
							{
								DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tli_node->data;

								if ( di->status == STATUS_DOWNLOADING )
								{
									g_tlv_last_tooltip_item = -2;	// Allow active downloads to update the tooltip if their item is rehovered.
								}

								/*// The 32-bit version of _snwprintf in ntdll.dll on Windows XP crashes when a %s proceeds two %llu.
								int tooltip_buffer_offset = __snwprintf( g_tlv_tooltip_buffer, 512, L"%s: %s\r\n%s: %llu / ", ST_V_Filename, di->shared_info->file_path + di->shared_info->filename_offset, ST_V_Downloaded, di->downloaded );

								if ( di->file_size > 0 )
								{
									tooltip_buffer_offset += __snwprintf( g_tlv_tooltip_buffer + tooltip_buffer_offset, 512 - tooltip_buffer_offset, L"%llu", di->file_size );
								}
								else
								{
									g_tlv_tooltip_buffer[ tooltip_buffer_offset++ ] = L'?';
								}

								__snwprintf( g_tlv_tooltip_buffer + tooltip_buffer_offset, 512 - tooltip_buffer_offset, L" bytes\r\n%s: %s", ST_V_Added, di->shared_info->w_add_time );*/

								if ( di->file_size > 0 )
								{
									__snwprintf( g_tlv_tooltip_buffer, 512, L"%s: %s\r\n%s: %I64u / %I64u bytes\r\n%s: %s", ST_V_Filename, di->shared_info->file_path + di->shared_info->filename_offset, ST_V_Downloaded, di->downloaded, di->file_size, ST_V_Added, di->shared_info->w_add_time );
								}
								else
								{
									__snwprintf( g_tlv_tooltip_buffer, 512, L"%s: %s\r\n%s: %I64u / ? bytes\r\n%s: %s", ST_V_Filename, di->shared_info->file_path + di->shared_info->filename_offset, ST_V_Downloaded, di->downloaded, ST_V_Added, di->shared_info->w_add_time );
								}
							}
						}

						tti.lpszText = g_tlv_tooltip_buffer;
					}

					_SendMessageW( g_hWnd_tlv_tooltip, TTM_SETTOOLINFO, 0, ( LPARAM )&tti );
					_SendMessageW( g_hWnd_tlv_tooltip, TTM_TRACKACTIVATE, TRUE, ( LPARAM )&tti );
				}
			}
		}
		break;

		case WM_MOUSELEAVE:
		{
			if ( g_tlv_is_tracking == TRUE && !g_is_dragging )
			{
				g_tlv_is_tracking = FALSE;

				g_tlv_last_tooltip_item = -1;

				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;
				//tti.lpszText = NULL;
				_SendMessageW( g_hWnd_tlv_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			if ( g_tlv_is_drag_and_drop )
			{
				IDropSource *DropSource = ( IDropSource * )Create_IDropSource( hWnd );
				if ( DropSource != NULL )
				{
					FORMATETC fetc;
					fetc.cfFormat = ( CLIPFORMAT )CF_TREELISTVIEW;
					fetc.ptd = NULL;
					fetc.dwAspect = DVASPECT_CONTENT;
					fetc.lindex = -1;
					fetc.tymed = TYMED_HGLOBAL;

					STGMEDIUM stgm;
					_memzero( &stgm, sizeof( STGMEDIUM ) );

					IDataObject *DataObject = ( IDataObject * )Create_IDataObject( &fetc, &stgm, 1 );
					if ( DataObject != NULL )
					{
						DWORD dwEffect;
						DWORD ret = _DoDragDrop( DataObject, DropSource, DROPEFFECT_MOVE, &dwEffect );

						if ( ret == DRAGDROP_S_DROP )
						{
							if ( g_drag_and_drop_cti != NULL )
							{
								HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_category_move, ( void * )g_drag_and_drop_cti, 0, NULL );
								if ( thread != NULL )
								{
									CloseHandle( thread );
								}
							}
						}
						/*else if ( ret == DRAGDROP_S_CANCEL )
						{
						}*/

						DataObject->Release();
					}

					DropSource->Release();
				}

				_ReleaseCapture();
				g_tlv_is_drag_and_drop = false;

				break;
			}

			// Allow the lasso selection if we're not in the edit mode, or we've dragged from outside of the item list.
			if ( !g_in_list_edit_mode || g_drag_start_index == -1 || g_drag_pos.x > g_header_width )
			{
				HandleMouseMovement( hWnd );
			}
			else
			{
				HandleMouseDrag( hWnd );
			}

			int tracking_x = GET_X_LPARAM( lParam );
			int tracking_y = GET_Y_LPARAM( lParam );

			if ( tracking_x != g_tracking_x || tracking_y != g_tracking_y )
			{
				g_tracking_x = tracking_x;
				g_tracking_y = tracking_y;

				TOOLINFO tti;
				_memzero( &tti, sizeof( TOOLINFO ) );
				tti.cbSize = sizeof( TOOLINFO );
				tti.hwnd = hWnd;
				tti.uId = ( UINT_PTR )hWnd;
				_SendMessageW( g_hWnd_tlv_tooltip, TTM_TRACKACTIVATE, FALSE, ( LPARAM )&tti );

				TRACKMOUSEEVENT tmi;
				tmi.cbSize = sizeof( TRACKMOUSEEVENT );
				tmi.dwFlags = TME_HOVER | TME_LEAVE;
				tmi.hwndTrack = hWnd;
				tmi.dwHoverTime = 2000;
				g_tlv_is_tracking = _TrackMouseEvent( &tmi );
			}
		}
		break;

		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			_SetFocus( hWnd );

			if ( msg == WM_LBUTTONDOWN && _GetKeyState( VK_MENU ) & 0x8000 )
			{
				POINT pt;
				_GetCursorPos( &pt );
				_ScreenToClient( hWnd, &pt );

				int pick_index = ( pt.y - g_client_rc.top ) / g_row_height;
				if ( pick_index < 0 )
				{
					pick_index = 0;
				}
				pick_index += g_first_visible_index;

				// From the first visible node, get the node that was clicked.
				TREELISTNODE *tli_node = TLV_GetFirstVisibleItem();
				for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
				{
					tli_node = TLV_NextNode( tli_node, false );
				}

				if ( tli_node != NULL && tli_node->flag & TLVS_SELECTED )
				{
					_SetCapture( hWnd );
					g_tlv_is_drag_and_drop = true;
				}
			}
			else
			{
				HandleMouseClick( hWnd, ( msg == WM_LBUTTONDOWN ? false : true ) );
			}
		}
		break;

		case WM_LBUTTONDBLCLK:
		{
			_SetFocus( hWnd );
			_GetCursorPos( &g_drag_pos );
			_ScreenToClient( hWnd, &g_drag_pos );

			// Hide the edit textbox if it's displayed and we've clicked outside the Filename column.
			if ( g_show_edit_state != 0 )
			{
				_KillTimer( hWnd, IDT_EDIT_TIMER );

				g_show_edit_state = 0;
			}

			int pick_index = ( g_drag_pos.y - g_client_rc.top ) / g_row_height;
			if ( pick_index < 0 )
			{
				pick_index = 0;
			}
			pick_index += g_first_visible_index;

			// From the first visible node, get the node that was clicked.
			TREELISTNODE *tli_node = TLV_GetFirstVisibleItem();

			for ( int i = g_first_visible_index; i < pick_index && tli_node != NULL; ++i )
			{
				tli_node = TLV_NextNode( tli_node, false );
			}

			if ( tli_node != NULL )
			{
				if ( ( _GetKeyState( VK_CONTROL ) & 0x8000 ) &&
					 ( _GetKeyState( VK_SHIFT ) & 0x8000 ) )
				{
					HandleCommand( hWnd, MENU_OPEN_DIRECTORY );
				}
				else
				{
					if ( tli_node->child_count > 0 && !( _GetKeyState( VK_CONTROL ) & 0x8000 ) )
					{
						TLV_ExpandCollapseParent( tli_node, pick_index, !tli_node->is_expanded );

						HandleWindowChange( hWnd );

						_InvalidateRect( hWnd, &g_client_rc, TRUE );
					}
					else
					{
						DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tli_node->data;
						if ( di != NULL &&
							 IS_STATUS( di->status,
								STATUS_CONNECTING |
								STATUS_DOWNLOADING |
								STATUS_COMPLETED |
								STATUS_STOPPED |
								STATUS_TIMED_OUT |
								STATUS_FAILED |
								STATUS_SKIPPED |
								STATUS_AUTH_REQUIRED |
								STATUS_PROXY_AUTH_REQUIRED ) )
						{
							HandleCommand( hWnd, MENU_UPDATE_DOWNLOAD );
						}
					}
				}
			}

			if ( !in_worker_thread )
			{
				UpdateMenus( true );
			}
		}
		break;

		case WM_KILLFOCUS:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			if ( msg == WM_LBUTTONUP && g_tlv_is_drag_and_drop )
			{
				_ReleaseCapture();
				g_tlv_is_drag_and_drop = false;
			}

			if ( g_scroll_timer_active )
			{
				_KillTimer( hWnd, IDT_SCROLL_TIMER );

				g_scroll_timer_active = false;

				g_v_scroll_direction = 0;

				g_v_scroll_line_amount = 0;

				g_h_scroll_direction = 0;

				g_h_scroll_line_amount = 0;
			}

			if ( g_is_dragging )
			{
				if ( g_in_list_edit_mode )
				{
					if ( g_draw_drag )
					{
						if ( g_focused_node == NULL || g_focused_node->parent == NULL )
						{
							TREELISTNODE *selected_list = NULL;

							int expanded_item_count = 0;
							int focused_offset = 0;
							int current_offset = g_first_selection_index;

							int t_first_selection_index = -1;
							int first_selection_index = -1;
							TREELISTNODE *first_selection_node = NULL;
							int last_selection_index = -1;
							TREELISTNODE *last_selection_node = NULL;

							int first_visible_index = g_first_visible_index;
							int first_visible_root_index = g_first_visible_root_index;

							TREELISTNODE *tmp_node = NULL;
							TREELISTNODE *node = g_first_selection_node;

							int first_static_selection_index = -1;
							TREELISTNODE *first_static_selection_node = NULL;
							int last_static_selection_index = -1;
							TREELISTNODE *last_static_selection_node = NULL;

							while ( node != NULL )
							{
								if ( node->flag & TLVS_SELECTED )
								{
									if ( t_first_selection_index == -1 )
									{
										// The true first selection.
										// If we use a modifier key (Ctrl or Shift), then the first selection might not actually be selected.
										g_first_selection_node = node;
										g_first_selection_index = t_first_selection_index = current_offset;
									}

									if ( node->parent == NULL )
									{
										++expanded_item_count;

										tmp_node = node->next;

										TREELISTNODE *rem_node = node;

										// Set a new focused node if we're removing the current one.
										bool set_focused = false;
										if ( rem_node == g_focused_node )
										{
											if ( rem_node == g_tree_list )
											{
												g_focused_node = rem_node->next;
											}
											else
											{
												g_focused_node = rem_node->prev;

												set_focused = true;
											}
										}

										TLV_RemoveNode( &g_tree_list, rem_node );
										TLV_AddNode( &selected_list, rem_node, -1 );

										if ( set_focused )
										{
											g_focused_node = g_focused_node->next;
										}

										// Adjust our first visible indices if we've removed items from above it.
										if ( current_offset <= g_first_visible_index )
										{
											if ( first_visible_root_index > 0 )
											{
												--first_visible_root_index;
											}

											if ( first_visible_index > 0 )
											{
												--first_visible_index;
											}
										}

										if ( current_offset < g_focused_index )
										{
											// Number of selected items between the first movable selected index and the focused index.
											focused_offset = expanded_item_count;

											if ( node->is_expanded )
											{
												focused_offset += node->child_count;
											}
										}

										if ( first_selection_index == -1 )
										{
											first_selection_node = node;
										}

										// If we've dragged the item to the end of the list.
										if ( g_focused_index > g_expanded_item_count )
										{
											first_selection_index = g_expanded_item_count - expanded_item_count;
										}
										else
										{
											first_selection_index = g_focused_index - focused_offset;
										}

										last_selection_node = node;

										// If we've dragged the item to the end of the list.
										if ( g_focused_index > g_expanded_item_count )
										{
											last_selection_index = g_expanded_item_count - expanded_item_count;
										}
										else
										{
											last_selection_index = first_selection_index + ( expanded_item_count - 1 );
										}

										// Handle the expanded children of this item if it has any.
										if ( node->child != NULL && node->is_expanded )
										{
											node = node->child;

											while ( node != NULL )
											{
												++expanded_item_count;

												++current_offset;

												// Adjust our first visible index if we've removed items from above it.
												if ( current_offset <= g_first_visible_index )
												{
													if ( first_visible_index > 0 )
													{
														--first_visible_index;
													}
												}

												if ( node->flag & TLVS_SELECTED )
												{
													last_selection_node = node;
													last_selection_index = first_selection_index + ( expanded_item_count - 1 );
												}

												if ( node == g_last_selection_node )
												{
													rem_node = node;
												}

												node = node->next;
											}
										}

										if ( rem_node == g_last_selection_node )
										{
											break;
										}

										node = tmp_node;

										++current_offset;

										continue;
									}
									else	// A child is selected, but not its parent.
									{
										if ( current_offset < g_focused_index && first_static_selection_index == -1 )
										{
											first_static_selection_node = node;
											first_static_selection_index = current_offset - expanded_item_count;
										}

										if ( current_offset > g_focused_index )
										{
											last_static_selection_node = node;
											last_static_selection_index = current_offset - expanded_item_count;	// Exclude all selected items above this index.
										}
									}
								}

								if ( node == g_last_selection_node )
								{
									break;
								}

								if ( node->parent != NULL && node->next == NULL )
								{
									node = node->parent->next;
								}
								else if ( node->child != NULL && node->is_expanded )
								{
									node = node->child;
								}
								else
								{
									node = node->next;
								}

								++current_offset;
							}

							// If we have unmovable items at the beginning or end of our selection, then set the first/last selection items to them.
							if ( first_static_selection_node != NULL )
							{
								first_selection_node = first_static_selection_node;
								first_selection_index = first_static_selection_index;
							}

							if ( last_static_selection_node != NULL )
							{
								last_selection_node = last_static_selection_node;
								last_selection_index = last_static_selection_index + expanded_item_count;	// Adjusts the index to include the selected items below its initial index.
							}

							if ( g_tree_list == NULL && selected_list != NULL )
							{
								g_tree_list = selected_list;
							}
							else if ( g_tree_list != NULL && selected_list != NULL )
							{
								// Move the selected list to the bottom of the tree.
								if ( g_focused_node == NULL )
								{
									TREELISTNODE *tln_last = ( g_tree_list->prev != NULL ? g_tree_list->prev : g_tree_list );	// The last item in the tree list.

									tln_last->next = selected_list;
									g_tree_list->prev = ( selected_list->prev != NULL ? selected_list->prev : selected_list );
									selected_list->prev = tln_last;

									g_focused_index = ( g_expanded_item_count - expanded_item_count );
								}
								else
								{
									node = ( selected_list->prev != NULL ? selected_list->prev : selected_list );	// The last item in the selected list.

									// Move the selected list to the top of the tree.
									if ( g_focused_node == g_tree_list )
									{
										node->next = g_tree_list;
										selected_list->prev = ( g_tree_list->prev != NULL ? g_tree_list->prev : g_tree_list );
										g_tree_list->prev = node;

										g_tree_list = selected_list;

										g_focused_index = 0;
									}
									else	// Move the selected list somewhere in the middle of the tree.
									{
										node->next = g_focused_node;
										selected_list->prev = g_focused_node->prev;
										if ( g_focused_node->prev != NULL )
										{
											g_focused_node->prev->next = selected_list;
										}
										g_focused_node->prev = node;

										// Adjust the index if there were items removed before the focused index.
										if ( g_focused_index > g_first_selection_index )
										{
											g_focused_index -= focused_offset;
										}
									}
								}

								g_focused_node = selected_list;

								// Adjust our visible indices.
								if ( g_focused_index - first_visible_index >= g_visible_item_count )
								{
									// If our first visible item is the last child of a parent, then increase the root index.
									if ( g_first_visible_node->parent != NULL )
									{
										if ( g_first_visible_node->next == NULL )
										{
											++first_visible_root_index;
										}
									}
									else	// If our first visible item has no children (and isn't a child), then increase the root index.
									{
										if ( g_first_visible_node->child_count == 0 )
										{
											++first_visible_root_index;
										}
									}

									++first_visible_index;
								}

								// Iterate back from our focused node until we get the new first visible node.
								g_first_visible_node = g_focused_node;
								g_first_visible_index = first_visible_index;

								for ( int i = g_focused_index; g_first_visible_node != g_tree_list && i > first_visible_index; --i )
								{
									g_first_visible_node = TLV_PrevNode( g_first_visible_node, false );
								}

								g_first_visible_root_index = first_visible_root_index;

								g_base_selected_node = g_focused_node;
								g_base_selected_index = g_focused_index;

								g_first_selection_node = first_selection_node;
								g_first_selection_index = first_selection_index;

								g_last_selection_node = last_selection_node;
								g_last_selection_index = last_selection_index;

								HandleWindowChange( hWnd );
							}
						}
					}
					else
					{
						// Clear the selection, exception for the focused item, if we just click on it rather than drag.
						bool ctrl_down = ( _GetKeyState( VK_CONTROL ) & 0x8000 ) ? true : false;
						bool shift_down = ( _GetKeyState( VK_SHIFT ) & 0x8000 ) && !ctrl_down ? true : false;
						if ( !ctrl_down && !shift_down && g_drag_start_index != -1 && g_drag_pos.x <= g_header_width && g_focused_node != NULL )
						{
							// Remove the selection flag for all items in the list.
							TLV_ClearSelected( false, false );

							g_selected_count = 1;

							g_first_selection_node = g_focused_node;
							g_first_selection_index = g_focused_index;

							g_last_selection_node = g_focused_node;
							g_last_selection_index = g_focused_index;

							if ( g_focused_node != NULL )
							{
								g_focused_node->flag = ( TLVS_SELECTED | TLVS_FOCUSED );
							}

							g_drag_start_index = g_focused_index;

							g_base_selected_node = g_focused_node;
							g_base_selected_index = g_focused_index;
						}
					}
				}

				TLV_ClearDrag();

				_InvalidateRect( hWnd, &g_client_rc, TRUE );
			}

			g_mod_key_active = false;

			if ( msg == WM_LBUTTONUP )
			{
				if ( g_show_edit_state == 1 )
				{
					if ( g_hWnd_edit_box == NULL )
					{
						_SetTimer( hWnd, IDT_EDIT_TIMER, GetDoubleClickTime(), ( TIMERPROC )EditTimerProc );
					}
				}

				g_mod_first_selection_index = -1;
				g_mod_first_selection_node = NULL;

				g_mod_last_selection_index = -1;
				g_mod_last_selection_node = NULL;

				if ( !in_worker_thread )
				{
					UpdateMenus( true );
				}
			}
			else if ( msg == WM_RBUTTONUP )
			{
				if ( !in_worker_thread )
				{
					UpdateMenus( true );
				}

				POINT pt;
				pt.x = LOWORD( lParam );
				pt.y = HIWORD( lParam );
				_ClientToScreen( hWnd, &pt );

				_TrackPopupMenu( g_hMenuSub_download, 0, pt.x, pt.y, 0, hWnd, NULL );
			}
			else if ( msg == WM_MBUTTONUP )
			{
				if ( !in_worker_thread )
				{
					UpdateMenus( true );
				}

				if ( g_focused_node != NULL && g_focused_node->data != NULL && ( ( DOWNLOAD_INFO * )g_focused_node->data )->status == STATUS_COMPLETED )
				{
					HandleCommand( hWnd, MENU_OPEN_DIRECTORY );
				}
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			HandleCommand( hWnd, LOWORD( wParam ) );

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_ITEMCHANGING:
				{
					//NMHEADER *nmh = ( NMHEADER * )lParam;

					int arr[ NUM_COLUMNS ];
					_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );

					RECT rc;
					_memzero( &rc, sizeof( RECT ) );
					_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, arr[ g_total_columns - 1 ], ( LPARAM )&rc );

					g_header_width = rc.right;

					SCROLLINFO si;
					_memzero( &si, sizeof( SCROLLINFO ) );
					si.cbSize = sizeof( SCROLLINFO );
					si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
					_GetScrollInfo( hWnd, SB_HORZ, &si );

					bool adjust_column = ( ( si.nPos + si.nPage ) <= ( unsigned int )si.nMax ? false : true );

					HandleWindowChange( hWnd, false, adjust_column );

					_InvalidateRect( hWnd, &g_client_rc, TRUE );
				}
				break;

				case HDN_BEGINDRAG:
				case HDN_BEGINTRACK:
				{
					_SetFocus( hWnd );
				}
				break;

				case HDN_ENDDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					// Prevent the # columns from moving and the other columns from becoming the first column.
					if ( nmh->iItem == 0 || nmh->pitem->iOrder == 0 )
					{
						// Make sure the # columns are visible.
						if ( *download_columns[ COLUMN_NUM ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, download_columns, NUM_COLUMNS );
							return TRUE;
						}
					}

					_InvalidateRect( hWnd, &g_client_rc, TRUE );
				}
				break;

				case HDN_ENDTRACK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					int index = GetVirtualIndexFromColumnIndex( nmh->iItem, download_columns, NUM_COLUMNS );

					if ( index != -1 )
					{
						*download_columns_width[ index ] = _UNSCALE_( nmh->pitem->cxy, dpi_tlv );
					}
				}
				break;

				case HDN_ITEMCLICK:
				{
					_SetFocus( hWnd );

					NMHEADER *nmh = ( NMHEADER * )lParam;

					int index = GetVirtualIndexFromColumnIndex( nmh->iItem, download_columns, NUM_COLUMNS );

					// Change the format of the items in the column if Ctrl is held while clicking the column.
					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						// Change the size column info.
						if ( index != -1 )
						{
							if ( index == COLUMN_NUM )
							{
								TLV_ExpandCollapseAll( GetKeyState( VK_SHIFT ) & 0x8000 ? false : true );

								HandleWindowChange( hWnd );

								_InvalidateRect( hWnd, &g_client_rc, TRUE );
							}
							else if ( index == COLUMN_DOWNLOAD_SPEED )
							{
								if ( cfg_t_down_speed >= SIZE_FORMAT_AUTO )
								{
									cfg_t_down_speed = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_down_speed;
								}
								_InvalidateRect( hWnd, &g_client_rc, TRUE );
							}
							else if ( index == COLUMN_DOWNLOAD_SPEED_LIMIT )
							{
								if ( cfg_t_speed_limit >= SIZE_FORMAT_AUTO )
								{
									cfg_t_speed_limit = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_speed_limit;
								}
								_InvalidateRect( hWnd, &g_client_rc, TRUE );
							}
							else if ( index == COLUMN_DOWNLOADED )
							{
								if ( cfg_t_downloaded >= SIZE_FORMAT_AUTO )
								{
									cfg_t_downloaded = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_downloaded;
								}
								_InvalidateRect( hWnd, &g_client_rc, TRUE );
							}
							else if ( index == COLUMN_FILE_SIZE )
							{
								if ( cfg_t_file_size >= SIZE_FORMAT_AUTO )
								{
									cfg_t_file_size = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_file_size;
								}
								_InvalidateRect( hWnd, &g_client_rc, TRUE );
							}
						}
					}
					else	// Normal column click. Sort the items in the column.
					{
						HDITEM hdi;
						_memzero( &hdi, sizeof( HDITEM ) );
						hdi.mask = HDI_FORMAT | HDI_ORDER;
						_SendMessageW( nmh->hdr.hwndFrom, HDM_GETITEM, ( WPARAM )nmh->iItem, ( LPARAM )&hdi );

						if ( cfg_sorted_column_index != index )
						{
							cfg_sorted_column_index = index;

							g_download_history_changed = true;
						}

						// The number column doesn't get sorted and its iOrder is always 0, so let's remove any sort arrows that are active.
						if ( hdi.iOrder == 0 && *download_columns[ COLUMN_NUM ] != -1 )
						{
							// Remove the sort format for all columns.
							for ( unsigned char i = 1; _SendMessageW( nmh->hdr.hwndFrom, HDM_GETITEM, i, ( LPARAM )&hdi ) == TRUE; ++i )
							{
								// Remove sort up and sort down
								hdi.fmt = hdi.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
								_SendMessageW( nmh->hdr.hwndFrom, HDM_SETITEM, i, ( LPARAM )&hdi );
							}

							cfg_sorted_direction = 0;

							break;
						}

						SORT_INFO si;
						si.column = index;
						si.hWnd = nmh->hdr.hwndFrom;

						if ( HDF_SORTUP & hdi.fmt )	// Column is sorted upward.
						{
							si.direction = 0;	// Now sort down.

							// Sort down
							hdi.fmt = hdi.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
							_SendMessageW( nmh->hdr.hwndFrom, HDM_SETITEM, ( WPARAM )nmh->iItem, ( LPARAM )&hdi );
						}
						else if ( HDF_SORTDOWN & hdi.fmt )	// Column is sorted downward.
						{
							si.direction = 1;	// Now sort up.

							// Sort up
							hdi.fmt = hdi.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
							_SendMessageW( nmh->hdr.hwndFrom, HDM_SETITEM, nmh->iItem, ( LPARAM )&hdi );
						}
						else	// Column has no sorting set.
						{
							// Remove the sort format for all columns.
							for ( unsigned char i = 0; _SendMessageW( nmh->hdr.hwndFrom, HDM_GETITEM, i, ( LPARAM )&hdi ) == TRUE; ++i )
							{
								// Remove sort up and sort down
								hdi.fmt = hdi.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
								_SendMessageW( nmh->hdr.hwndFrom, HDM_SETITEM, i, ( LPARAM )&hdi );
							}

							// Read current the format from the clicked column
							_SendMessageW( nmh->hdr.hwndFrom, HDM_GETITEM, nmh->iItem, ( LPARAM )&hdi );

							si.direction = 0;	// Start the sort going down.

							// Sort down to start.
							hdi.fmt = hdi.fmt | HDF_SORTDOWN;
							_SendMessageW( nmh->hdr.hwndFrom, HDM_SETITEM, nmh->iItem, ( LPARAM )&hdi );
						}

						if ( cfg_sorted_direction != si.direction )
						{
							cfg_sorted_direction = si.direction;

							g_download_history_changed = true;
						}

						if ( _GetKeyState( VK_SHIFT ) & 0x8000 )
						{
							TREELISTNODE *tli_parent = g_tree_list;
							while ( tli_parent != NULL )
							{
								if ( tli_parent->child != NULL )
								{
									TLV_MergeSort( &tli_parent->child, tli_parent->child_count, &merge_compare, ( void * )&si );
								}

								tli_parent = tli_parent->next;
							}
						}
						else
						{
							TLV_MergeSort( &g_tree_list, g_total_parent_item_nodes/*g_root_item_count*/, &merge_compare, ( void * )&si );
						}

						TLV_CleanupSort();
					}
				}
				break;

				case HDN_DIVIDERDBLCLICK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					wchar_t tbuf[ 128 ];
					RECT rc;
					HDITEM hdi;
					int largest_width = _SCALE_TLV_( 20 );

					int arr[ NUM_COLUMNS ];
					_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );

					int last_column_index = arr[ g_total_columns - 1 ];

					int virtual_index = GetVirtualIndexFromColumnIndex( nmh->iItem, download_columns, NUM_COLUMNS );

					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						//largest_width = LVSCW_AUTOSIZE_USEHEADER;

						// Is the that last column?
						if ( nmh->iItem == arr[ g_total_columns - 1 ] )
						{
							_memzero( &rc, sizeof( RECT ) );
							_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, nmh->iItem, ( LPARAM )&rc );

							largest_width = ( g_client_rc.right - _SCALE_TLV_( 1 ) ) - rc.left;
						}

						_memzero( &hdi, sizeof( HDITEM ) );
						hdi.mask = HDI_TEXT;
						hdi.cchTextMax = 128;
						hdi.pszText = tbuf;
						if ( _SendMessageW( g_hWnd_tlv_header, HDM_GETITEM, nmh->iItem, ( LPARAM )&hdi ) == TRUE )
						{
							HDC hDC = _GetDC( g_hWnd_tlv_header );

							HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
							_DeleteObject( ohf );

							rc.bottom = rc.left = rc.right = rc.top = 0;

							SIZE size;
							if ( _GetTextExtentPoint32W( hDC, hdi.pszText, lstrlenW( hdi.pszText ), &size ) != FALSE )
							{
								int width = size.cx + _SCALE_TLV_( 6 ) + _SCALE_TLV_( 6 );	// 6 + 6 padding.
								if ( width > largest_width )
								{
									largest_width = width;
								}
							}

							_ReleaseDC( g_hWnd_tlv_header, hDC );
						}
					}
					else
					{
						OffsetVirtualIndices( arr, download_columns, NUM_COLUMNS, g_total_columns );
						int column_index = -1;
						for ( char i = 0; i < g_total_columns; ++i )
						{
							if ( arr[ i ] == virtual_index )
							{
								column_index = i;
								break;
							}
						}

						if ( column_index == 0 )
						{
							largest_width += g_glyph_size.cx;
						}

						if ( virtual_index != COLUMN_FILE_TYPE )	// File Type
						{
							int index = g_first_visible_index;
							int index_end = g_visible_item_count + index;

							int root_count = g_first_visible_root_index + 1;
							int child_count = g_first_visible_index - TLV_GetParentIndex( g_first_visible_node, g_first_visible_index );

							bool switch_fonts = ( tlv_even_row_font_settings.lf.lfHeight != tlv_odd_row_font_settings.lf.lfHeight );

							HDC hDC = _GetDC( hWnd );

							if ( !switch_fonts )
							{
								HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
								_DeleteObject( ohf );
							}

							TREELISTNODE *tln = TLV_GetFirstVisibleItem();

							for ( ; index <= index_end && tln != NULL; ++index )
							{
								if ( switch_fonts )
								{
									HFONT *hFont = ( index & 1 ? &tlv_even_row_font_settings.font : &tlv_odd_row_font_settings.font );

									HFONT ohf = ( HFONT )_SelectObject( hDC, *hFont );
									if ( ohf != tlv_even_row_font_settings.font && ohf != tlv_odd_row_font_settings.font )
									{
										_DeleteObject( ohf );
									}
								}

								DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
								if ( di != NULL )
								{
									wchar_t *buf = GetDownloadInfoString( di, virtual_index, root_count, child_count, tbuf, 128 );

									if ( buf == NULL )
									{
										tbuf[ 0 ] = L'\0';
										buf = tbuf;
									}

									rc.bottom = rc.left = rc.right = rc.top = 0;

									_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

									int width = ( rc.right - rc.left ) + _SCALE_TLV_( 5 ) + _SCALE_TLV_( 5 );	// 5 + 5 padding.
									if ( column_index == 0 )
									{
										width += g_glyph_size.cx;
									}
									if ( width > largest_width )
									{
										largest_width = width;
									}
								}

								tln = TLV_NextNode( tln, false );

								if ( tln != NULL && tln->parent != NULL )
								{
									++child_count;
								}
								else
								{
									++root_count;

									child_count = 0;
								}
							}

							_ReleaseDC( hWnd, hDC );
						}
					}

					_memzero( &hdi, sizeof( HDITEM ) );
					hdi.mask = HDI_WIDTH;
					hdi.cxy = largest_width;
					_SendMessageW( g_hWnd_tlv_header, HDM_SETITEM, nmh->iItem, ( LPARAM )&hdi );

					*download_columns_width[ virtual_index ] = largest_width;

					_memzero( &rc, sizeof( RECT ) );
					_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, last_column_index, ( LPARAM )&rc );

					g_header_width = rc.right;

					HandleWindowChange( hWnd );

					_InvalidateRect( hWnd, &g_client_rc, TRUE );

					return TRUE;
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_tlv_header )
					{
						POINT p;
						_GetCursorPos( &p );

						_TrackPopupMenu( g_hMenuSub_column, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

#ifdef ENABLE_DARK_MODE
				case NM_CUSTOMDRAW:
				{
					if ( g_use_dark_mode )
					{
						NMCUSTOMDRAW *nmcd = ( NMCUSTOMDRAW * )lParam;
						switch ( nmcd->dwDrawStage )
						{
							case CDDS_PREPAINT:
							{
								return CDRF_NOTIFYITEMDRAW;
							}
							break;

							case CDDS_ITEMPREPAINT:
							{
								_SetTextColor( nmcd->hdc, dm_color_window_text );

								return CDRF_DODEFAULT;
							}
							break;
						}
					}
					else
					{
						return _DefWindowProcW( hWnd, msg, wParam, lParam );
					}
				}
				break;
#endif
			}

			return FALSE;
		}
		break;

		case WM_ERASEBKGND:
		{
			// We'll handle the background drawing.
			return TRUE;
		}
		break;

		case WM_PAINT:
		{
			DrawTreeListView( hWnd );

			return 0;
		}
		break;

		case WM_NCCALCSIZE:
		{
			// Draw our scrollbars if there's any.
			LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

			if ( cfg_show_toolbar )
			{
				if ( wParam == FALSE )
				{
					RECT *pRect = ( RECT * )lParam;
					++pRect->top;
				}
				else// if ( wParam == TRUE )
				{
					NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
					++nccsp->rgrc[ 0 ].top;
				}
			}

			if ( cfg_show_categories )
			{
				if ( wParam == FALSE )
				{
					RECT *pRect = ( RECT * )lParam;
					++pRect->left;
				}
				else// if ( wParam == TRUE )
				{
					NCCALCSIZE_PARAMS *nccsp = ( NCCALCSIZE_PARAMS * )lParam;
					++nccsp->rgrc[ 0 ].left;
				}
			}

			return ret;
		}
		break;

		case WM_NCPAINT:
		{
			// Draw our scrollbars if there's any.
			LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

			if ( cfg_show_toolbar )
			{
				RECT rc;
				_GetWindowRect( hWnd, &rc );

				HDC hDC = _GetWindowDC( hWnd );
				HPEN line_color;
#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_SOLID, 1, ( COLORREF )_GetSysColor( COLOR_3DSHADOW ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hDC, line_color );
				_DeleteObject( old_color );

				_MoveToEx( hDC, 0, 0, NULL );
				_LineTo( hDC, rc.right - rc.left, 0 );
				_DeleteObject( line_color );

				_ReleaseDC( hWnd, hDC );
			}

			if ( cfg_show_categories )
			{
				RECT rc;
				_GetWindowRect( hWnd, &rc );

				HDC hDC = _GetWindowDC( hWnd );
				HPEN line_color;
#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_SOLID, 1, ( COLORREF )_GetSysColor( COLOR_3DSHADOW ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hDC, line_color );
				_DeleteObject( old_color );

				_MoveToEx( hDC, 0, 0, NULL );
				_LineTo( hDC, 0, rc.bottom - rc.top );
				_DeleteObject( line_color );

				_ReleaseDC( hWnd, hDC );
			}

			return ret;
		}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			// Window has been minimized.
			WINDOWPOS *wp = ( WINDOWPOS * )lParam;
			if ( wp->cx == 0 && wp->cy == 0 )
			{
				break;
			}

			g_size_changed = true;

			HandleWindowChange( hWnd );

			_InvalidateRect( hWnd, NULL, TRUE );

			return 0;
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			g_skip_window_change = false;

			g_size_changed = true;

			HandleWindowChange( hWnd );

			_InvalidateRect( hWnd, NULL, TRUE );

			// There is no return value.
		}
		break;

		case WM_DPICHANGED_BEFOREPARENT:
		{
			g_skip_window_change = true;

			UINT last_dpi_tlv = current_dpi_tlv;
			current_dpi_tlv = __GetDpiForWindow( hWnd );

			g_glyph_offset = _SCALE_TLV_( GLYPH_OFFSET );

			if ( g_hTheme != NULL )
			{
				_CloseThemeData( g_hTheme );

				g_hTheme = _OpenThemeData( hWnd, L"TreeView" );

				g_glyph_size.cx = 0;
				g_glyph_size.cy = 0;

				if ( g_hTheme != NULL )
				{
					_GetThemePartSize( g_hTheme, NULL, TVP_GLYPH, GLPS_OPENED, NULL, TS_DRAW, &g_glyph_size );
				}
			}

			// The font will not have changed here, only its height.
			tlv_odd_row_font_settings.lf.lfHeight = _SCALE_TLV_( cfg_odd_row_font_settings.lf.lfHeight );
			_DeleteObject( tlv_odd_row_font_settings.font );
			tlv_odd_row_font_settings.font = _CreateFontIndirectW( &tlv_odd_row_font_settings.lf );

			// The font will not have changed here, only its height.
			tlv_even_row_font_settings.lf.lfHeight = _SCALE_TLV_( cfg_even_row_font_settings.lf.lfHeight );
			_DeleteObject( tlv_even_row_font_settings.font );
			tlv_even_row_font_settings.font = _CreateFontIndirectW( &tlv_even_row_font_settings.lf );

			AdjustRowHeight();

			_DeleteObject( hFont_tlv );
			hFont_tlv = UpdateFont( current_dpi_tlv );

			_SendMessageW( g_hWnd_tlv_header, WM_SETFONT, ( WPARAM )hFont_tlv, 0 );

			if ( cfg_show_column_headers )
			{
				WINDOWPOS wp;
				_memzero( &wp, sizeof ( WINDOWPOS ) );
				RECT rc;
				rc.top = rc.left = 0;
				rc.bottom = _SCALE_TLV_( 100 );
				rc.right = g_header_width;
				HDLAYOUT hdl;
				hdl.prc = &rc;
				hdl.pwpos = &wp;
				_SendMessageW( g_hWnd_tlv_header, HDM_LAYOUT, 0, ( LPARAM )&hdl );

				g_header_height = ( wp.cy - wp.y );
			}

			int arr[ NUM_COLUMNS ];
			int arr2[ NUM_COLUMNS ];

			_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );

			_memcpy_s( arr2, sizeof( int ) * NUM_COLUMNS, arr, sizeof( int ) * NUM_COLUMNS );

			// Offset the virtual indices to match the actual index.
			OffsetVirtualIndices( arr2, download_columns, NUM_COLUMNS, g_total_columns );

			RECT rc;

			HDITEM hdi;
			_memzero( &hdi, sizeof( HDITEM ) );
			hdi.mask = HDI_WIDTH;

			for ( unsigned char i = 0; i < g_total_columns; ++i )
			{
				// Get the width of the column header.
				_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, arr[ i ], ( LPARAM )&rc );

				hdi.cxy = _SCALE2_( ( rc.right - rc.left ), dpi_tlv );
				_SendMessageW( g_hWnd_tlv_header, HDM_SETITEM, arr[ i ], ( LPARAM )&hdi );
			}

			// There is no return value.
		}
		break;

		case WM_DESTROY:
		{
			// Delete our font.
			_DeleteObject( hFont_tlv );

			if ( tlv_even_row_font_settings.font != NULL ){ _DeleteObject( tlv_even_row_font_settings.font ); }
			if ( tlv_odd_row_font_settings.font != NULL ){ _DeleteObject( tlv_odd_row_font_settings.font ); }

			if ( g_hbm != NULL )
			{
				_DeleteObject( g_hbm );
			}

			if ( g_tlv_tooltip_buffer != NULL )
			{
				GlobalFree( g_tlv_tooltip_buffer );
			}

			TLV_FreeTree( g_tree_list );

			// Close the theme.
			if ( g_hTheme != NULL )
			{
				_CloseThemeData( g_hTheme );
			}
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
