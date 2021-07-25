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

/*
	Dark Mode APIs:			https://github.com/ysc3839/win32-darkmode
	Menu Bar structures:	https://github.com/adzm/win32-custom-menubar-aero-theme
*/

#ifndef _DARK_MODE_H
#define _DARK_MODE_H

//#define ENABLE_DARK_MODE

#ifdef ENABLE_DARK_MODE


#include "lite_gdi32.h"
#include "lite_uxtheme.h"

#include "dwmapi.h"

//

#define DM_UPDATE_CHECK_URL			"https://raw.githubusercontent.com/erickutcher/httpdownloader/master/HTTP_Downloader/dm_version.txt"

//

#define WINDOWS_BUILD_1809			17763
#define WINDOWS_BUILD_1903			18362
#define WINDOWS_BUILD_1909			18363
#define WINDOWS_BUILD_20H1			18985
#define WINDOWS_BUILD_2004			19041
#define WINDOWS_BUILD_DEV_WIN11		21996	// Windows 11

//

/*
enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attr;
	PVOID pvData;
	SIZE_T cbData;
};

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};
*/

// 1903 18362
enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

//

// Menu Bar

// Window messages related to menu bar drawing.
#define WM_UAHDESTROYWINDOW		0x0090	// handled by DefWindowProc
#define WM_UAHDRAWMENU			0x0091	// lParam is UAHMENU
#define WM_UAHDRAWMENUITEM		0x0092	// lParam is UAHDRAWMENUITEM
#define WM_UAHINITMENU			0x0093	// handled by DefWindowProc
#define WM_UAHMEASUREMENUITEM	0x0094	// lParam is UAHMEASUREMENUITEM
#define WM_UAHNCPAINTMENUPOPUP	0x0095	// handled by DefWindowProc

// Describes the sizes of the menu bar or menu item.
typedef union tagUAHMENUITEMMETRICS
{
	struct
	{
		DWORD cx;
		DWORD cy;
	} rgsizeBar[ 2 ];

	struct
	{
		DWORD cx;
		DWORD cy;
	} rgsizePopup[ 4 ];
} UAHMENUITEMMETRICS;

// Not really used in our case but part of the other structures.
typedef struct tagUAHMENUPOPUPMETRICS
{
	DWORD rgcx[ 4 ];
	DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
} UAHMENUPOPUPMETRICS;

// hMenu is the main window menu; hDC is the context to draw in.
typedef struct tagUAHMENU
{
	HMENU hMenu;
	HDC hDC;
	DWORD dwFlags; // No idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
} UAHMENU;

// Menu items are always referred to by iPosition here.
typedef struct tagUAHMENUITEM
{
	int iPosition; // 0-based position of menu item in menubar.
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

// The DRAWITEMSTRUCT contains the states of the menu items, as well as the position index of the item in the menu, which is duplicated in the UAHMENUITEM's iPosition as well.
typedef struct UAHDRAWMENUITEM
{
	DRAWITEMSTRUCT dis; // itemID looks uninitialized
	UAHMENU um;
	UAHMENUITEM umi;
} UAHDRAWMENUITEM;

// The MEASUREITEMSTRUCT is intended to be filled with the size of the item height appears to be ignored, but width can be modified.
typedef struct tagUAHMEASUREMENUITEM
{
	MEASUREITEMSTRUCT mis;
	UAHMENU um;
	UAHMENUITEM umi;
} UAHMEASUREMENUITEM;

//

// user32.dll
#ifdef USER32_USE_STATIC_LIB

#define _GetMenuItemInfoW				GetMenuItemInfoW
#define _GetMenuBarInfo					GetMenuBarInfo
#define _EnumChildWindows				EnumChildWindows
#define _EnumThreadWindows				EnumThreadWindows
#define _GetClassNameW					GetClassNameW
//#define _RedrawWindow					RedrawWindow
#define _SetPropW						SetPropW
#define _RemovePropW					RemovePropW
#define _GetPropW						GetPropW

#else

typedef BOOL ( WINAPI *pGetMenuItemInfoW )( HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFO lpmii );
typedef BOOL ( WINAPI *pGetMenuBarInfo )( HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi );
typedef BOOL ( WINAPI *pEnumChildWindows )( HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam );
typedef BOOL ( WINAPI *pEnumThreadWindows )( DWORD dwThreadId, WNDENUMPROC lpfn, LPARAM lParam );
typedef int ( WINAPI *pGetClassNameW )( HWND hWnd, LPWSTR lpClassName, int nMaxCount );
//typedef BOOL ( WINAPI *pRedrawWindow )( HWND hWnd, const RECT *lprcUpdate, HRGN hrgnUpdate, UINT flags );
typedef BOOL ( WINAPI *pSetPropW )( HWND hWnd, LPCWSTR lpString, HANDLE hData );
typedef HANDLE ( WINAPI *pRemovePropW )( HWND hWnd, LPCWSTR lpString );
typedef HANDLE ( WINAPI *pGetPropW )( HWND hWnd, LPCWSTR lpString );

extern pGetMenuItemInfoW				_GetMenuItemInfoW;
extern pGetMenuBarInfo					_GetMenuBarInfo;
extern pEnumChildWindows				_EnumChildWindows;
extern pEnumThreadWindows				_EnumThreadWindows;
extern pGetClassNameW					_GetClassNameW;
//extern pRedrawWindow					_RedrawWindow;
extern pSetPropW						_SetPropW;
extern pRemovePropW						_RemovePropW;
extern pGetPropW						_GetPropW;

#endif

// Undocumented
//typedef BOOL ( WINAPI *pSetWindowCompositionAttribute )( HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA *pAttrData );

// Undocumented
//extern pSetWindowCompositionAttribute	_SetWindowCompositionAttribute;

// comctl32.dll
#ifdef COMDLG32_USE_STATIC_LIB

#define _ImageList_Draw					ImageList_Draw
#define _ImageList_SetImageCount		ImageList_SetImageCount
//#define _ImageList_GetIcon			ImageList_GetIcon
#define _SetWindowSubclass				SetWindowSubclass
#define _DefSubclassProc				DefSubclassProc
#define _RemoveWindowSubclass			RemoveWindowSubclass

#else

typedef BOOL ( WINAPI *pImageList_Draw )( HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle );
typedef BOOL ( WINAPI *pImageList_SetImageCount )( HIMAGELIST himl, UINT uNewCount );
//typedef HICON ( WINAPI *pImageList_GetIcon )( HIMAGELIST himl, int i, UINT flags );
typedef BOOL ( WINAPI *pSetWindowSubclass )( HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData );
typedef LRESULT ( WINAPI *pDefSubclassProc )( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
typedef BOOL ( WINAPI *pRemoveWindowSubclass )( HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass );

extern pImageList_Draw					_ImageList_Draw;
extern pImageList_SetImageCount			_ImageList_SetImageCount;
//extern pImageList_GetIcon				_ImageList_GetIcon;
extern pSetWindowSubclass				_SetWindowSubclass;
extern pDefSubclassProc					_DefSubclassProc;
extern pRemoveWindowSubclass			_RemoveWindowSubclass;

#endif

//

// gdi32.dll
#ifdef GDI32_USE_STATIC_LIB

#define _ExcludeClipRect				ExcludeClipRect
#define _SetBkColor						SetBkColor
#define _Polygon						Polygon
#define _ExtCreatePen					ExtCreatePen

#else

typedef int ( WINAPI *pExcludeClipRect )( HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect );
typedef COLORREF ( WINAPI *pSetBkColor )( HDC hdc, COLORREF crColor );
typedef BOOL ( WINAPI *pPolygon )( HDC hdc, const POINT *apt, int cpt );
typedef HPEN ( WINAPI *pExtCreatePen )( DWORD iPenStyle, DWORD cWidth, const LOGBRUSH *plbrush, DWORD cStyle, const DWORD *pstyle );

extern pExcludeClipRect					_ExcludeClipRect;
extern pSetBkColor						_SetBkColor;
extern pPolygon							_Polygon;
extern pExtCreatePen					_ExtCreatePen;

typedef HRGN ( WINAPI *pCreateRectRgn )( int x1, int y1, int x2, int y2 );
typedef int ( WINAPI *pCombineRgn )( HRGN hrgnDst, HRGN hrgnSrc1, HRGN hrgnSrc2, int iMode );
typedef HRGN ( WINAPI *pCreateRectRgnIndirect )( const RECT *lprect );

#endif

//

// ntdll.dll
// Undocumented
typedef void ( WINAPI *pRtlGetNtVersionNumbers )( LPDWORD major, LPDWORD minor, LPDWORD build );		// In ntdll.dll

// Undocumented
extern pRtlGetNtVersionNumbers					_RtlGetNtVersionNumbers;

//

// uxtheme.dll
#ifdef UXTHEME_USE_STATIC_LIB

#define _SetWindowTheme							SetWindowTheme
#define _DrawThemeTextEx						DrawThemeTextEx
//#define _GetThemeRect							GetThemeRect

#define _BufferedPaintInit						BufferedPaintInit
#define _BufferedPaintUnInit					BufferedPaintUnInit
#define _BufferedPaintRenderAnimation			BufferedPaintRenderAnimation
#define _BeginBufferedAnimation					BeginBufferedAnimation
#define _EndBufferedAnimation					EndBufferedAnimation
#define _BufferedPaintStopAllAnimations			BufferedPaintStopAllAnimations

#else

typedef HRESULT ( WINAPI *pSetWindowTheme )( HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList );
typedef HRESULT ( WINAPI *pDrawThemeTextEx )( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int cchText, DWORD dwTextFlags, LPRECT pRect, const DTTOPTS *pOptions );
//typedef HRESULT ( WINAPI *pGetThemeRect )( HTHEME hTheme, int iPartId, int iStateId, int iPropId, LPRECT pRect );

typedef HRESULT ( WINAPI *pBufferedPaintInit )();
typedef HRESULT ( WINAPI *pBufferedPaintUnInit )();
typedef BOOL ( WINAPI *pBufferedPaintRenderAnimation )( HWND hwnd, HDC hdcTarget );
typedef HANIMATIONBUFFER ( WINAPI *pBeginBufferedAnimation )( HWND hwnd, HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, BP_ANIMATIONPARAMS *pAnimationParams, HDC *phdcFrom, HDC *phdcTo );
typedef HRESULT ( WINAPI *pEndBufferedAnimation )( HANIMATIONBUFFER hbpAnimation, BOOL fUpdateTarget );
typedef HRESULT ( WINAPI *pBufferedPaintStopAllAnimations )( HWND hwnd );

extern pSetWindowTheme							_SetWindowTheme;
extern pDrawThemeTextEx							_DrawThemeTextEx;
//extern pGetThemeRect							_GetThemeRect;

extern pBufferedPaintInit						_BufferedPaintInit;
extern pBufferedPaintUnInit						_BufferedPaintUnInit;
extern pBufferedPaintRenderAnimation			_BufferedPaintRenderAnimation;
extern pBeginBufferedAnimation					_BeginBufferedAnimation;
extern pEndBufferedAnimation					_EndBufferedAnimation;
extern pBufferedPaintStopAllAnimations			_BufferedPaintStopAllAnimations;

#endif

// Undocumented
// 1809 17763
//typedef HTHEME ( WINAPI *pOpenNcThemeData )( HWND hWnd, LPCWSTR pszClassList );						// ordinal 49
//typedef void ( WINAPI *pRefreshImmersiveColorPolicyState )();											// ordinal 104
//typedef bool ( WINAPI *pGetIsImmersiveColorUsingHighContrast )( IMMERSIVE_HC_CACHE_MODE mode );		// ordinal 106
//typedef bool ( WINAPI *pShouldAppsUseDarkMode )();													// ordinal 132
typedef bool ( WINAPI *pAllowDarkModeForWindow )( HWND hWnd, bool allow );								// ordinal 133
typedef bool ( WINAPI *pAllowDarkModeForApp )( bool allow );											// ordinal 135, in 1809
//typedef void ( WINAPI *pFlushMenuThemes )();															// ordinal 136
//typedef bool ( WINAPI *pIsDarkModeAllowedForWindow )( HWND hWnd );									// ordinal 137

// 1903 18362
//typedef bool ( WINAPI *pShouldSystemUseDarkMode )();													// ordinal 138
typedef PreferredAppMode ( WINAPI *pSetPreferredAppMode )( PreferredAppMode appMode );					// ordinal 135, in 1903
//typedef bool ( WINAPI *pIsDarkModeAllowedForApp )();													// ordinal 139

// Undocumented
// 1809 17763
//extern pOpenNcThemeData						_OpenNcThemeData;
//extern pRefreshImmersiveColorPolicyState		_RefreshImmersiveColorPolicyState;
//extern pGetIsImmersiveColorUsingHighContrast	_GetIsImmersiveColorUsingHighContrast;
//extern pShouldAppsUseDarkMode					_ShouldAppsUseDarkMode;
extern pAllowDarkModeForWindow					_AllowDarkModeForWindow;
extern pAllowDarkModeForApp						_AllowDarkModeForApp;
//extern pFlushMenuThemes						_FlushMenuThemes;
//extern pIsDarkModeAllowedForWindow			_IsDarkModeAllowedForWindow;
// 1903 18362
//extern pShouldSystemUseDarkMode				_ShouldSystemUseDarkMode;
extern pSetPreferredAppMode						_SetPreferredAppMode;
//extern pIsDarkModeAllowedForApp				_IsDarkModeAllowedForApp;

//

// dwmapi.dll
#ifdef DWMAPI_USE_STATIC_LIB

#define _DwmSetWindowAttribute			DwmSetWindowAttribute

#else

typedef HRESULT ( WINAPI *pDwmSetWindowAttribute )( HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute );

extern pDwmSetWindowAttribute			_DwmSetWindowAttribute;

#endif

//

bool InitDarkMode();
bool UninitDarkMode();

BOOL CALLBACK EnumChildProc( HWND hWnd, LPARAM lParam );
BOOL CALLBACK EnumTLWProc( HWND hWnd, LPARAM lParam );
BOOL CALLBACK EnumMsgBoxChildProc( HWND hWnd, LPARAM lParam );

//

extern bool g_use_dark_mode;

extern DWORD g_dm_buildNumber;

extern HTHEME g_hTheme_menu;
extern HBRUSH g_hBrush_window_background;
extern HBRUSH g_hBrush_edit_background;

extern COLORREF dm_color_window_text;
extern COLORREF dm_color_window_background;
extern COLORREF dm_color_window_border;

extern COLORREF dm_color_edit_background;
extern COLORREF dm_color_edit_border_hover;
extern COLORREF dm_color_edit_border_enabled;
extern COLORREF dm_color_edit_border_disabled;

extern COLORREF dm_color_button_normal;
extern COLORREF dm_color_button_hover;
extern COLORREF dm_color_button_pressed;
extern COLORREF dm_color_button_border_hover;
extern COLORREF dm_color_button_border_enabled;
extern COLORREF dm_color_button_border_disabled;

extern COLORREF dm_color_ud_button_border_enabled;
extern COLORREF dm_color_ud_button_border_disabled;
extern COLORREF dm_color_arrow;

extern COLORREF dm_color_menu_hover;
//extern COLORREF dm_color_menu_border;

extern COLORREF dm_color_focus_rectangle;

extern COLORREF dm_color_list_highlight;

extern COLORREF dm_color_listbox_border;

extern COLORREF dm_color_listview_grid;

extern COLORREF dm_color_toolbar_separator;

extern DWORD CBS_DARK_MODE;
extern DWORD LBS_DARK_MODE;

#else

#define CBS_DARK_MODE		0
#define LBS_DARK_MODE		0

#endif

#endif
