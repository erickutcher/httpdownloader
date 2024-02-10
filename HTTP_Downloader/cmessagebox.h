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

#ifndef _CMESSAGEBOX_H
#define _CMESSAGEBOX_H

#define WM_DESTROY_CMSGBOX		WM_APP + 1000

// Return values.
#define CMBIDFAIL				0
#define CMBIDOK					1
#define CMBIDCANCEL				2		// Not supported.
#define CMBIDABORT				3		// Not supported.
#define CMBIDRETRY				4		// Not supported.
#define CMBIDIGNORE				5		// Not supported.
#define CMBIDYES				6
#define CMBIDNO					7
#define CMBIDCLOSE				8		// Not supported.
#define CMBIDHELP				9		// Not supported.
#define CMBIDTRYAGAIN			10		// Not supported.
#define CMBIDCONTINUE			11
#define CMBIDTIMEOUT			32000	// Not supported.

#define CMBIDOKALL				1000
#define CMBIDYESALL				60
#define CMBIDYESADD				600
#define CMBIDNOALL				70
#define CMBIDCONTINUEALL		110
#define CMBIDRENAME				12
#define CMBIDRENAMEALL			120
#define CMBIDOVERWRITE			13
#define CMBIDOVERWRITEALL		130
#define CMBIDSKIP				14
#define CMBIDSKIPALL			140
#define CMBIDRESTART			15
#define CMBIDRESTARTALL			150

// CMessageBox styles.
#define CMB_OK					0x00000000L
#define CMB_OKCANCEL			0x00000001L	// Not supported.
#define CMB_ABORTRETRYIGNORE	0x00000002L	// Not supported.
#define CMB_YESNOCANCEL			0x00000003L	// Not supported.
#define CMB_YESNO				0x00000004L
#define CMB_RETRYCANCEL			0x00000005L	// Not supported.
#define CMB_CANCELTRYCONTINUE	0x00000006L	// Not supported.
#define CMB_OKALL				0x00000007L
#define CMB_YESNOALL			0x00000008L
#define CMB_RENAMEOVERWRITESKIPALL	0x00000009L
#define CMB_CONTINUERESTARTSKIPALL	0x0000000AL

// CMessageBox icon styles.
#define CMB_ICONHAND			0x00000010L
#define CMB_ICONQUESTION		0x00000020L
#define CMB_ICONEXCLAMATION		0x00000030L
#define CMB_ICONASTERISK		0x00000040L

#define CMB_ICONWARNING			CMB_ICONEXCLAMATION
#define CMB_ICONERROR			CMB_ICONHAND

#define CMB_ICONINFORMATION		CMB_ICONASTERISK
#define CMB_ICONSTOP			CMB_ICONHAND

struct CMSGBOX_INFO
{
	wchar_t *cmsgbox_message;
	HWND hWnd_checkbox;
	UINT type;
	bool use_theme;
};

struct CPROMPT_INFO
{
	void *data;
	wchar_t *cp_message;
	HWND hWnd_host;
	HWND hWnd_key_algorithm;
	HWND hWnd_key_fingerprints;
	HWND hWnd_key_size;
	HWND hWnd_checkbox;
	UINT type;
};

bool InitializeCMessageBox( HINSTANCE hInstance );

LRESULT CALLBACK CustomMessageBoxWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

int CMessageBoxW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType );
int CPromptW( HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, void *data );

extern CRITICAL_SECTION cmessagebox_prompt_cs;	// Guard access to the tray menu when creating a cmessagebox.

#endif
