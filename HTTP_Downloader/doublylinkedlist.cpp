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

#include "doublylinkedlist.h"

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

DoublyLinkedList *DLL_CreateNode( void *data )
{
	DoublyLinkedList *dll = ( DoublyLinkedList * )GlobalAlloc( GPTR, sizeof( DoublyLinkedList ) );
	if ( dll != NULL )
	{
		dll->next = NULL;
		dll->prev = NULL;
		dll->data = data;
	}

	return dll;
}

void DLL_RemoveNode( DoublyLinkedList **head, DoublyLinkedList *node )
{
	if ( *head != NULL && node != NULL )
	{
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

void DLL_AddNode( DoublyLinkedList **head, DoublyLinkedList *node, int position )
{
	if ( node == NULL )
	{
		return;
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
		DoublyLinkedList *last_node = *head;
		DoublyLinkedList *current_node = ( *head )->next;
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
