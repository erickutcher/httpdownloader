/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2024 Eric Kutcher

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

#ifndef _DOUBLYLINKEDLIST_H
#define _DOUBLYLINKEDLIST_H

struct DoublyLinkedList
{
	DoublyLinkedList *prev;
	DoublyLinkedList *next;
	void *data;
};


DoublyLinkedList *DLL_CreateNode( void *data );

void DLL_RemoveNode( DoublyLinkedList **head, DoublyLinkedList *node );
void DLL_AddNode( DoublyLinkedList **head, DoublyLinkedList *node, int position );

#endif
