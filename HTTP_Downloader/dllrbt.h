/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2021 Eric Kutcher

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

// This Red-Black tree is based on an implementation by Thomas Niemann.
// Refer to "Sorting and Searching Algorithms: A Cookbook"
// The implementation has been modified to include a doubly-linked list.

#ifndef DLLRBT_H
#define DLLRBT_H

typedef enum
{
	DLLRBT_STATUS_OK,
	DLLRBT_STATUS_MEM_EXHAUSTED,
	DLLRBT_STATUS_DUPLICATE_KEY,
	DLLRBT_STATUS_KEY_NOT_FOUND,
	DLLRBT_STATUS_TREE_NOT_FOUND
} dllrbt_status;

typedef enum
{ 
	BLACK,
	RED
} node_color;

typedef struct node
{
	struct node *left;		// Left child
	struct node *right;		// Right child
	struct node *parent;	// Parent
	node_color color;		// Node color (BLACK, RED)
	void *key;				// Key used for searching
	void *val;				// User data

	struct node *previous;	// Prevoius node
	struct node *next;		// Next node
} node_type;

typedef void dllrbt_iterator;
typedef void dllrbt_tree;

// Create a doubly-linked list red-black tree and set the comparison function.
dllrbt_tree *dllrbt_create( int ( *compare )( void *a, void *b ) );

// Insert a key/value pair.
dllrbt_status dllrbt_insert( dllrbt_tree *tree, void *key, void *value );

// Removes a node from the tree. Does not free the key/value pair.
dllrbt_status dllrbt_remove( dllrbt_tree *tree, dllrbt_iterator *i );

// Returns an iterator or value associated with a key.
dllrbt_iterator *dllrbt_find( dllrbt_tree *tree, void *key, bool return_value );

// Returns the head of the doubly-linked list.
node_type *dllrbt_get_head( dllrbt_tree *tree );

// Returns the tail of the doubly-linked list.
node_type *dllrbt_get_tail( dllrbt_tree *tree );

// Destroy the tree recursively using the red-black tree. Does not free the key/value pair.
void dllrbt_delete_recursively( dllrbt_tree *tree );

// Destroy the tree iteratively using the doubly-linked list. Does not free the key/value pair.
//void dllrbt_delete_iteratively( dllrbt_tree *tree );

// Get the total number of nodes in the doubly-linked list.
unsigned int dllrbt_get_node_count( dllrbt_tree *tree );

#endif
