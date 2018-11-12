// scsitree.cpp
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include "resource.h"
#include "aspi32.h"

// function prototypes
LONG CALLBACK CPlApplet(HWND, UINT, LONG, LONG);
BOOL APIENTRY ScsiDlgProc(HWND, UINT, UINT, LONG);

//
BOOL FontSelectDlg( HWND hWnd, LPLOGFONT lplf );
void SetScsiTree( HWND hDlg );
void AddScsiTree( HWND hDlg, HTREEITEM htiHA, int nHANo, LPSTR lpszHAInq );
HTREEITEM AddScsiTreeItem( HWND hDlg, HTREEITEM hParent, HTREEITEM hInsAfter, LPSTR lpszText, int nImage );
void SetScsiList( HWND hDlg );
// void SetTreeImages( HWND hDlg );

//
HANDLE hModule = NULL;
char szCtlPanel[30];
CAspi32 aspi;
BOOL fShowAll;
LOGFONT lfTree;
HFONT hfTree;
HIMAGELIST hImglist;

//
//	FUNCTION: DllMain(PVOID, ULONG, PCONTEXT)
//	PURPOSE: Win 32 Initialization DLL
//	COMMENTS:
//
BOOL WINAPI DllMain( IN PVOID hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL )
{
	if( ulReason != DLL_PROCESS_ATTACH ){
		return TRUE;
	} else {
		hModule = hmod;
	}

	return TRUE;
}

//
//	FUNCTION: CPIApplet(HWND, UINT, LONG, LONG)
//	PURPOSE: Processes messages for control panel applets
//	COMMENTS:
//
LONG CALLBACK CPlApplet( HWND hwndCPL, UINT uMsg, LONG lParam1, LONG lParam2 )
{
	int iApplet;
	LPNEWCPLINFO lpNewCPlInfo;
	static iInitCount = 0;

	switch( uMsg ){
	case CPL_INIT:			// first message, sent once
		if( ! iInitCount ){
		//	InitApplet()
			lstrcpy( szCtlPanel, "Control Panel" );
			// return FALSE;
		}
		iInitCount++;
		return TRUE;
	case CPL_GETCOUNT:		// second message, sent once
		return 1;
		break;
	case CPL_NEWINQUIRE:	// third message, sent once per app
		lpNewCPlInfo = (LPNEWCPLINFO)lParam2;
		iApplet = lParam1;
		lpNewCPlInfo->dwSize = sizeof(NEWCPLINFO);
		lpNewCPlInfo->dwFlags = 0;
		lpNewCPlInfo->dwHelpContext = 0;
		lpNewCPlInfo->lData = 0;
		lpNewCPlInfo->hIcon = LoadIcon( (HINSTANCE)hModule, (LPCTSTR)MAKEINTRESOURCE(SCSI_ICON) );
		lpNewCPlInfo->szHelpFile[0] = '\0';
		lstrcpy( lpNewCPlInfo->szName, "SCSI Tree" );
		lstrcpy( lpNewCPlInfo->szInfo, "SCSI Tree (Windows95 Only)" );
		break;
	case CPL_SELECT:		// application icon selected
		break;
	case CPL_DBLCLK:		// application icon double-clicked
		iApplet = lParam1;
		DialogBox( (HINSTANCE)hModule, MAKEINTRESOURCE(SCSI_TREE_DLG), hwndCPL, (DLGPROC)ScsiDlgProc );
		break;
	case CPL_STOP:			// sent once per app. before CPL_EXIT
		break;
	case CPL_EXIT:			// sent once before FreeLibrary called
		iInitCount--;
	//	if( ! iInitCount )
	//		TermApplet();
		break;
	default:
		break;
	}

	return 0;
}

//
//	FUNCTION: AmpDlgProc
//	PURPOSE: Processes messages sent to the Amp applet.
//	COMMENTS:
//		This dialog simply puts up a box and has an OK key.
//		It doesn't do anything except display.
//
BOOL APIENTRY ScsiDlgProc( HWND hDlg, UINT message, UINT wParam, LONG lParam )
{
	HICON hIcon;
	HMENU hMenu;
	HWND hwndTree;
	HWND hwndList;
	int cx, cy;
	RECT rc;
	char szBuf[256];

	switch (message){
	case WM_INITDIALOG:
		// InitCommonControls();
		//
		hIcon = LoadIcon( (HINSTANCE)hModule, (LPCTSTR)MAKEINTRESOURCE(SCSI_ICON) );
		SendMessage( hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon );
		//
		hMenu = GetSystemMenu( hDlg, FALSE );
		AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
		AppendMenu( hMenu, MF_STRING, ID_OPT_REFRESH, "R&efresh" );
		AppendMenu( hMenu, MF_STRING, ID_OPT_FONT,    "&Font..." );
		//
		ZeroMemory( &lfTree, sizeof(LOGFONT) );
		if( GetPrivateProfileStruct( "SCSI Tree", "font", &lfTree, sizeof(LOGFONT), "control.ini" ) == FALSE ){
			GetObject( GetStockObject( ANSI_FIXED_FONT ), sizeof(LOGFONT), (LPVOID)&lfTree );
		}
		hfTree = CreateFontIndirect( &lfTree );
		SendMessage( GetDlgItem( hDlg, SCSI_TREE ), WM_SETFONT, (WPARAM)hfTree, MAKELPARAM(TRUE,0) );
		//
	//	if( GetPrivateProfileInt( "SCSI Tree", "FixedFont", TRUE, "control.ini" ) != FALSE ){
	//		SendMessage( GetDlgItem( hDlg, SCSI_TREE ), WM_SETFONT, (WPARAM)GetStockObject( ANSI_FIXED_FONT ), MAKELPARAM( TRUE, 0 ));
	//	}
		//
		cx = GetPrivateProfileInt( "SCSI Tree", "cx", -1, "control.ini" );
		cy = GetPrivateProfileInt( "SCSI Tree", "cy", -1, "control.ini" );
		//
		if( cx > 0 && cy > 0 ){
			SetWindowPos( hDlg, NULL, 0, 0, cx, cy, SWP_NOMOVE );
		}
		//
	//	SetTreeImages( hDlg );
		//
		SetScsiTree( hDlg );
		SetScsiList( hDlg );
		//
		return( TRUE );
	case WM_COMMAND:
		if( LOWORD(wParam) ){
			EndDialog( hDlg, TRUE );
			return( TRUE );
		}
		return( FALSE );
	case WM_SIZE:
		cx = LOWORD(lParam);
		cy = HIWORD(lParam);
		hwndTree = GetDlgItem( hDlg, SCSI_TREE );
		hwndList = GetDlgItem( hDlg, SCSI_LIST );
	//	MoveWindow( hwndTree, 0, 0, cx, cy/2, TRUE );
	//	MoveWindow( hwndList, 0, cy/2, cx,  cy/2,  TRUE );
		MoveWindow( hwndTree, 0, 0, cx, cy, TRUE );
		MoveWindow( hwndList, 0, 0, 0, 0, TRUE );
		return( TRUE );
	case WM_CLOSE:
	//	if( hImglist ){
	//		ImageList_Destroy( hImglist );
	//	}
		//
		GetWindowRect( hDlg, &rc );
		wsprintf( szBuf, "%d", rc.right - rc.left );
		WritePrivateProfileString( "SCSI Tree", "cx", szBuf, "control.ini" );
		wsprintf( szBuf, "%d", rc.bottom - rc.top );
		WritePrivateProfileString( "SCSI Tree", "cy", szBuf, "control.ini" );
		//
		WritePrivateProfileStruct( "SCSI Tree", "font", &lfTree, sizeof(LOGFONT), "control.ini" );
		//
		DestroyWindow( hDlg );
		return( TRUE );
	case WM_SYSCOMMAND:
		switch( wParam ){
		case ID_OPT_REFRESH:
			SetScsiTree( hDlg );
			SetScsiList( hDlg );
			return( TRUE );
		case ID_OPT_FONT:
			if( FontSelectDlg( hDlg, &lfTree ) == TRUE ){
				DeleteObject( hfTree );
				hfTree = CreateFontIndirect( &lfTree );
				SendMessage( GetDlgItem( hDlg, SCSI_TREE ), WM_SETFONT, (WPARAM)hfTree, MAKELPARAM(TRUE,0) );
				SendMessage( hDlg, WM_SYSCOMMAND, ID_OPT_REFRESH, 0 );
			}
			return( TRUE );
		}
		break;
	}
	return( FALSE );
}

//
//	font select dialog
//
BOOL FontSelectDlg( HWND hWnd, LPLOGFONT lplf )
{
	CHOOSEFONT cf;

	cf.lStructSize		= sizeof(CHOOSEFONT);
	cf.hwndOwner		= hWnd;
	cf.hDC				= NULL;
	cf.lpLogFont		= lplf;
	cf.iPointSize		= 0;
	cf.Flags			= CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_LIMITSIZE;
	cf.rgbColors		= 0L;
	cf.lCustData		= 0L;
	cf.lpfnHook			= NULL;
	cf.hInstance		= NULL;
	cf.lpszStyle		= NULL;
	cf.nFontType		= SCREEN_FONTTYPE;
	cf.nSizeMin			= 6;
	cf.nSizeMax			= 72;
	cf.lpTemplateName	= NULL;

	if( ChooseFont( &cf ) == FALSE ){
		return( FALSE );
	}

	return( TRUE );
}

//
void SetScsiTree( HWND hDlg )
{
	int i;
	char szInq1[32];
	char szInq2[32];
	char szBuf[256];
	HTREEITEM htiHA;

	TreeView_DeleteAllItems( GetDlgItem( hDlg, SCSI_TREE ) ); 

//	fShowAll = GetPrivateProfileInt( "SCSI Tree", "ShowAll", FALSE, "control.ini" );
	if(( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0 ){
		fShowAll = TRUE;
	} else {
		fShowAll = FALSE;
	}

	if( aspi.GetHACount() == 0 ){
		AddScsiTreeItem( hDlg, NULL, TVI_ROOT, "No Host Adapter", 0 );
		return;
	}

	for( i = 0; i < aspi.nHACount; i++ ){
		if( aspi.GetHAInquiry( i, szInq1, szInq2 ) == TRUE ){
		//	wsprintf( szBuf, "ID:%u %s %s", aspi.GetHAScsiID( i ), szInq1, szInq2 );
			wsprintf( szBuf, "%s", szInq1 );
			htiHA = AddScsiTreeItem( hDlg, NULL, TVI_ROOT, szBuf, 0 );
			AddScsiTree( hDlg, htiHA, i, szInq2 );
		}
	}
}

//
void AddScsiTree( HWND hDlg, HTREEITEM htiHA, int nHANo, LPSTR lpszHAInq )
{
	int i, j;
	int nHAID;
	char szBuf[128];
	BYTE bCapa[16];
	DWORD dwCapaTotal;
	DWORD dwCapaSize;
	double doCapa;
	CScsiInq si;
	HTREEITEM htiSCSI;
	HTREEITEM htiLUN;

	nHAID = aspi.GetHAScsiID( nHANo );

	htiSCSI = TVI_FIRST;
	for( i = 0; i < 8; i++ ){
		if( i == nHAID ){
			wsprintf( szBuf, "ID:%u Host Adapter", i );
			htiSCSI = AddScsiTreeItem( hDlg, htiHA, htiSCSI, szBuf, 1 );
		//	AddScsiTreeItem( hDlg, htiSCSI, htiSCSI, lpszHAInq, 0 );
		} else {
			htiLUN = TVI_FIRST;
			for( j = 0; j < 8; j++ ){
				if( aspi.GetScsiInquiry( nHANo, i, j, SCSI_INQUIRY_LENGTH, (LPBYTE)szBuf ) == TRUE ){
					si.SplitInquiryData( (LPBYTE)szBuf );
					if( htiLUN == TVI_FIRST ){
						wsprintf( szBuf, "ID:%u %s %s %s", i, si.szVenderID, si.szProductID, si.szRevision );
						htiSCSI = AddScsiTreeItem( hDlg, htiHA, htiSCSI, szBuf, 1 );
					}
					if( si.nAnsiVer == 0 ){
						wsprintf( szBuf, "LUN:%u %s", j, si.szDevTypeStr );
					} else {
						wsprintf( szBuf, "LUN:%u %s SCSI-%u", j, si.szDevTypeStr, si.nAnsiVer );
					}
					if( si.fRemovable == TRUE ){
						lstrcat( szBuf, " Removable" );
					}
					htiLUN = AddScsiTreeItem( hDlg, htiSCSI, htiLUN, szBuf, 2 );
					if( si.nDevType == 0 || si.nDevType == 4 || si.nDevType == 5 || si.nDevType == 7 ){
						lstrcpy( szBuf, "Not Ready" );
						if( aspi.TestUnitReady( nHANo, i, j ) == TRUE ){
							lstrcpy( szBuf, "No Capacity" );
							if( aspi.ReadCapacity( nHANo, i, j, (LPBYTE)bCapa ) == TRUE ){
								dwCapaTotal = (DWORD)((WORD)( bCapa[0] * 256 + bCapa[1] ));
								dwCapaTotal <<= 16;
								dwCapaTotal |= (DWORD)((WORD)( bCapa[2] * 256 + bCapa[3] ));
								dwCapaSize = (DWORD)((WORD)( bCapa[4] * 256 + bCapa[5] ));
								dwCapaSize <<= 16;
								dwCapaSize |= (DWORD)((WORD)( bCapa[6] * 256 + bCapa[7] ));
								doCapa = (double)dwCapaTotal * (double)dwCapaSize;
								doCapa /= (double)(1024*1024);
								wsprintf( szBuf, "Total %lu MB / Sector %lu Bytes", (DWORD)doCapa, dwCapaSize );
							}
						}
						AddScsiTreeItem( hDlg, htiLUN, htiLUN, szBuf, 3 );
					}
				} else {
					if( j == 0 && fShowAll != FALSE ){
						wsprintf( szBuf, "ID:%u N/A", i );
						htiSCSI = AddScsiTreeItem( hDlg, htiHA, htiSCSI, szBuf, 1 );
					}
				}
			}
		}
	}
}

//
HTREEITEM AddScsiTreeItem( HWND hDlg, HTREEITEM hParent, HTREEITEM hInsAfter, LPSTR lpszText, int nImage )
{
	HWND hwndTree; 
	HTREEITEM hInsert;
	TV_ITEM tvi;
	TV_INSERTSTRUCT tvins;

	tvi.mask			= TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.pszText			= lpszText;
	tvi.cchTextMax		= lstrlen( lpszText );
	//	tvi.iImage			= nImage;
	//	tvi.iSelectedImage	= nImage;
	tvi.iImage			= 0;
	tvi.iSelectedImage	= 0;

	tvins.item			= tvi;
	tvins.hInsertAfter	= hInsAfter;
	tvins.hParent		= hParent;

	hwndTree = GetDlgItem( hDlg, SCSI_TREE );
	hInsert = TreeView_InsertItem( hwndTree, &tvins );

	return( hInsert );
}

//
void SetScsiList( HWND hDlg )
{
//	int i;
//	char szInq[128];
//	char szBuf[256];

	HWND hwndList = GetDlgItem( hDlg, SCSI_LIST );
	ListView_DeleteAllItems( hwndList ); 

	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt  = LVCFMT_LEFT;

	lvc.iSubItem = 0;
	lvc.cx = 30;
	lvc.pszText = "HA";
	ListView_InsertColumn( hwndList, 0, &lvc );

	lvc.iSubItem = 1;
	lvc.cx = 30;
	lvc.pszText = "ID";
	ListView_InsertColumn( hwndList, 1, &lvc );

	lvc.iSubItem = 2;
	lvc.cx = 60;
	lvc.pszText = "Vender";
	ListView_InsertColumn( hwndList, 2, &lvc );

	lvc.iSubItem = 3;
	lvc.cx = 60;
	lvc.pszText = "Product";
	ListView_InsertColumn( hwndList, 3, &lvc );

	lvc.iSubItem = 4;
	lvc.cx = 40;
	lvc.pszText = "Rev";
	ListView_InsertColumn( hwndList, 4, &lvc );

	LV_ITEM lvi;

//	if( aspi.GetHACount() == 0 ){
		lvi.mask		= LVIF_TEXT | LVIF_STATE;
		lvi.state		= 0;      
		lvi.stateMask	= 0;  
		lvi.iItem		= 0;
		lvi.iSubItem	= 0;
		lvi.pszText		= "";
		ListView_InsertItem( hwndList, &lvi );
		ListView_SetItemText( hwndList, 0, 1, "No" );
		ListView_SetItemText( hwndList, 0, 2, "Host" );
		ListView_SetItemText( hwndList, 0, 3, "Adapter" );
		ListView_SetItemText( hwndList, 0, 4, "" );
//	} else {
//	}
}

//
/*
void SetTreeImages( HWND hDlg )
{
	HBITMAP hBitmap;

	hImglist = ImageList_Create( 16, 16, TRUE, 4, 0 );

	hBitmap = LoadBitmap( (HINSTANCE)hModule, MAKEINTRESOURCE(IDB_SCSI1) );
	if( hBitmap ){
		ImageList_AddMasked( hImglist, hBitmap, RGB(255,0,255) );
		DeleteObject( hBitmap );
	}

	hBitmap = LoadBitmap( (HINSTANCE)hModule, MAKEINTRESOURCE(IDB_SCSI2) );
	if( hBitmap ){
		ImageList_AddMasked( hImglist, hBitmap, RGB(255,0,255) );
		DeleteObject( hBitmap );
	}

	hBitmap = LoadBitmap( (HINSTANCE)hModule, MAKEINTRESOURCE(IDB_SCSI3) );
	if( hBitmap ){
		ImageList_AddMasked( hImglist, hBitmap, RGB(255,0,255) );
		DeleteObject( hBitmap );
	}

	hBitmap = LoadBitmap( (HINSTANCE)hModule, MAKEINTRESOURCE(IDB_SCSI4) );
	if( hBitmap ){
		ImageList_AddMasked( hImglist, hBitmap, RGB(255,0,255) );
		DeleteObject( hBitmap );
	}

	if( ImageList_GetImageCount( hImglist ) < 4 ){
		ImageList_Destroy( hImglist );
		return;
	}

	TreeView_SetImageList( GetDlgItem( hDlg, SCSI_TREE ), hImglist, 0 );
}
*/

//
