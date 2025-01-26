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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "globals.h"
#include "string_tables.h"
#include "dark_mode.h"

//

extern HWND g_hWnd_options_tree;

extern HWND g_hWnd_sftp_fps_tab;
extern HWND g_hWnd_sftp_keys_tab;

extern bool g_t_tray_icon;	// Let's the Web Server options know we can display notifications.

//

extern UINT current_dpi_options;
extern UINT last_dpi_options;
extern HFONT hFont_options;

#define _SCALE_O_( x )						_SCALE_( ( x ), dpi_options )

//

extern WNDPROC FocusLBProc;
extern WNDPROC FocusCBProc;
extern WNDPROC FocusComboProc;
extern WNDPROC FocusEditProc;
extern WNDPROC FocusBtnProc;

LRESULT CALLBACK FocusLBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK FocusCBSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK FocusComboSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK FocusEditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK FocusBtnSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

#endif
