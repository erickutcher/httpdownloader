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

#include "lite_dlls.h"
#include "lite_user32.h"

#ifndef USER32_USE_STATIC_LIB

	pBeginDeferWindowPos	_BeginDeferWindowPos;
	pBeginPaint				_BeginPaint;
	pCallWindowProcW		_CallWindowProcW;
	//pCharLowerA				_CharLowerA;
	//pCharLowerBuffA			_CharLowerBuffA;
	//pCharUpperA			_CharUpperA;
	pCheckMenuItem			_CheckMenuItem;
	pClientToScreen			_ClientToScreen;
	pCloseClipboard			_CloseClipboard;
	pCreateMenu				_CreateMenu;
	pCreatePopupMenu		_CreatePopupMenu;
	pCreateWindowExW		_CreateWindowExW;
	pDefWindowProcW			_DefWindowProcW;
	pDeferWindowPos			_DeferWindowPos;
	//pDeleteMenu				_DeleteMenu;
	pDestroyMenu			_DestroyMenu;
	pDestroyWindow			_DestroyWindow;
	pDispatchMessageW		_DispatchMessageW;
	pDrawEdge				_DrawEdge;
	pDrawIconEx				_DrawIconEx;
	//pDrawMenuBar				_DrawMenuBar;
	pDrawTextW				_DrawTextW;
	pEmptyClipboard			_EmptyClipboard;
	pEnableMenuItem			_EnableMenuItem;
	pEnableWindow			_EnableWindow;
	pEndDeferWindowPos		_EndDeferWindowPos;
	pEndPaint				_EndPaint;
	//pEnumChildWindows		_EnumChildWindows;
	pFillRect				_FillRect;
	pFlashWindow			_FlashWindow;
	//pGetClassLongPtrW		_GetClassLongPtrW;
	pGetClientRect			_GetClientRect;
	//pGetClipboardData		_GetClipboardData
	pGetCursorPos			_GetCursorPos;
	pGetDC					_GetDC;
	//pGetDlgItem				_GetDlgItem;
	pGetKeyState			_GetKeyState;
	//pGetMenuItemInfoW		_GetMenuItemInfoW;
	//pGetMessagePos			_GetMessagePos;
	pGetMessageW			_GetMessageW;
	pGetParent				_GetParent;
	//pGetSubMenu				_GetSubMenu;
	pGetSysColor			_GetSysColor;
	pGetSysColorBrush		_GetSysColorBrush;
	pGetSystemMetrics		_GetSystemMetrics;
	pGetWindowLongW			_GetWindowLongW;
	pGetWindowRect			_GetWindowRect;
	pInsertMenuItemW		_InsertMenuItemW;
	pInvalidateRect			_InvalidateRect;
	//pIsCharAlphaNumericA		_IsCharAlphaNumericA;
	pIsDialogMessageW		_IsDialogMessageW;
	pIsIconic				_IsIconic;
	pIsWindowVisible		_IsWindowVisible;
	pIsZoomed				_IsZoomed;
	pKillTimer				_KillTimer;
	pLoadCursorW				_LoadCursorW;
	pLoadIconW				_LoadIconW;
	pLoadImageW				_LoadImageW;
	//pMapWindowPoints			_MapWindowPoints;
	pMessageBoxA			_MessageBoxA;
	pMessageBoxW			_MessageBoxW;
	pOffsetRect				_OffsetRect;
	pOpenClipboard			_OpenClipboard;
	pPostMessageW			_PostMessageW;
	pPostQuitMessage		_PostQuitMessage;
	pRegisterClassExW		_RegisterClassExW;
	pRegisterClipboardFormatW	_RegisterClipboardFormatW;
	pReleaseCapture			_ReleaseCapture;
	pReleaseDC				_ReleaseDC;
	//pRemoveMenu				_RemoveMenu;
	pScreenToClient			_ScreenToClient;
	pScrollWindow			_ScrollWindow;
	pSendMessageA			_SendMessageA;
	pSendMessageW			_SendMessageW;
	pSendNotifyMessageW		_SendNotifyMessageW;
	pSetCapture				_SetCapture;
	//pSetClassLongPtrW		_SetClassLongPtrW;
	pSetClipboardData		_SetClipboardData;
	pSetCursor				_SetCursor;
	pSetFocus				_SetFocus;
	pSetForegroundWindow	_SetForegroundWindow;
	pSetLayeredWindowAttributes		_SetLayeredWindowAttributes;
	pSetMenu					_SetMenu;
	//pSetMenuItemInfoW		_SetMenuItemInfoW;
	//pSetRect					_SetRect;
	pSetScrollInfo			_SetScrollInfo;
	pSetScrollPos			_SetScrollPos;
	pSetTimer				_SetTimer;
	pSetWindowLongW			_SetWindowLongW;
	pSetWindowPos			_SetWindowPos;
	//pSetWindowTextW			_SetWindowTextW;
	pShowWindow				_ShowWindow;
	pSystemParametersInfoW	_SystemParametersInfoW;
	pTrackPopupMenu			_TrackPopupMenu;
	pTranslateMessage		_TranslateMessage;

	HMODULE hModule_user32 = NULL;

	unsigned char user32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeUser32()
	{
		if ( user32_state != USER32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_user32 = LoadLibraryDEMW( L"user32.dll" );

		if ( hModule_user32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_BeginDeferWindowPos, "BeginDeferWindowPos" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_BeginPaint, "BeginPaint" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CallWindowProcW, "CallWindowProcW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CharLowerA, "CharLowerA" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CharLowerBuffA, "CharLowerBuffA" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CharUpperA, "CharUpperA" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CheckMenuItem, "CheckMenuItem" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ClientToScreen, "ClientToScreen" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CloseClipboard, "CloseClipboard" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CreateMenu, "CreateMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CreatePopupMenu, "CreatePopupMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_CreateWindowExW, "CreateWindowExW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DefWindowProcW, "DefWindowProcW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DeferWindowPos, "DeferWindowPos" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DeleteMenu, "DeleteMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DestroyMenu, "DestroyMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DestroyWindow, "DestroyWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DispatchMessageW, "DispatchMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DrawEdge, "DrawEdge" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DrawIconEx, "DrawIconEx" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DrawMenuBar, "DrawMenuBar" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_DrawTextW, "DrawTextW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EmptyClipboard, "EmptyClipboard" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EnableMenuItem, "EnableMenuItem" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EnableWindow, "EnableWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EndDeferWindowPos, "EndDeferWindowPos" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EndPaint, "EndPaint" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_EnumChildWindows, "EnumChildWindows" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_FillRect, "FillRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_FlashWindow, "FlashWindow" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetClassLongPtrW, "GetClassLongPtrW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetClientRect, "GetClientRect" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetClipboardData, "GetClipboardData" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetCursorPos, "GetCursorPos" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetDC, "GetDC" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetDlgItem, "GetDlgItem" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetKeyState, "GetKeyState" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetMenuItemInfoW, "GetMenuItemInfoW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetMessagePos, "GetMessagePos" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetMessageW, "GetMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetParent, "GetParent" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetSubMenu, "GetSubMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetSysColor, "GetSysColor" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetSysColorBrush, "GetSysColorBrush" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetSystemMetrics, "GetSystemMetrics" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetWindowLongW, "GetWindowLongW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_GetWindowRect, "GetWindowRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_InsertMenuItemW, "InsertMenuItemW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_InvalidateRect, "InvalidateRect" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_IsCharAlphaNumericA, "IsCharAlphaNumericA" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_IsDialogMessageW, "IsDialogMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_IsIconic, "IsIconic" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_IsWindowVisible, "IsWindowVisible" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_IsZoomed, "IsZoomed" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_KillTimer, "KillTimer" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_LoadCursorW, "LoadCursorW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_LoadIconW, "LoadIconW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_LoadImageW, "LoadImageW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_MapWindowPoints, "MapWindowPoints" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_MessageBoxA, "MessageBoxA" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_MessageBoxW, "MessageBoxW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_OffsetRect, "OffsetRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_OpenClipboard, "OpenClipboard" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_PostMessageW, "PostMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_PostQuitMessage, "PostQuitMessage" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_RegisterClassExW, "RegisterClassExW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_RegisterClipboardFormatW, "RegisterClipboardFormatW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ReleaseCapture, "ReleaseCapture" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ReleaseDC, "ReleaseDC" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_RemoveMenu, "RemoveMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ScreenToClient, "ScreenToClient" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ScrollWindow, "ScrollWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SendMessageA, "SendMessageA" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SendMessageW, "SendMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SendNotifyMessageW, "SendNotifyMessageW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetCapture, "SetCapture" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetClassLongPtrW, "SetClassLongPtrW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetClipboardData, "SetClipboardData" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetCursor, "SetCursor" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetFocus, "SetFocus" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetForegroundWindow, "SetForegroundWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetLayeredWindowAttributes, "SetLayeredWindowAttributes" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetMenu, "SetMenu" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetMenuItemInfoW, "SetMenuItemInfoW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetRect, "SetRect" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetScrollInfo, "SetScrollInfo" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetScrollPos, "SetScrollPos" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetTimer, "SetTimer" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetWindowLongW, "SetWindowLongW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetWindowPos, "SetWindowPos" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SetWindowTextW, "SetWindowTextW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_ShowWindow, "ShowWindow" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_SystemParametersInfoW, "SystemParametersInfoW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_TrackPopupMenu, "TrackPopupMenu" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_user32, ( void ** )&_TranslateMessage, "TranslateMessage" ) )

		user32_state = USER32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeUser32()
	{
		if ( user32_state != USER32_STATE_SHUTDOWN )
		{
			user32_state = USER32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_user32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
