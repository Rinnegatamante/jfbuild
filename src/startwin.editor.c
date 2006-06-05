#ifndef RENDERTYPEWIN
#error Only for Windows
#endif

#include "build.h"
#include "editor.h"
#include "winlayer.h"
#include "compat.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#define _WIN32_IE 0x0300
#include <commctrl.h>
#include <stdio.h>

#include "startwin.editor.h"

static void PopulateVideoModeLists(int fs, HWND list2d, HWND list3d)
{
	int i,j;
	char buf[64];

	ComboBox_ResetContent(list2d);
	ComboBox_ResetContent(list3d);
	for (i=0; i<validmodecnt; i++) {
		if (validmode[i].fs != fs) continue;

		// all modes get added to the 3D mode list
		Bsprintf(buf, "%ldx%ld %dbpp", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp);
		j = ComboBox_AddString(list3d, buf);
		ComboBox_SetItemData(list3d, j, i);
		if (xdimgame == validmode[i].xdim && ydimgame == validmode[i].ydim && bppgame == validmode[i].bpp)
			ComboBox_SetCurSel(list3d, j);

		// only 8-bit modes get used for 2D
		if (validmode[i].bpp != 8) continue;
		Bsprintf(buf, "%ldx%ld", validmode[i].xdim, validmode[i].ydim);
		j = ComboBox_AddString(list2d, buf);
		ComboBox_SetItemData(list2d, j, i);
		if (xdim2d == validmode[i].xdim && ydim2d == validmode[i].ydim && 8 == validmode[i].bpp)
			ComboBox_SetCurSel(list2d, j);
	}
}

static INT_PTR CALLBACK ConfigPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		//case WM_INITDIALOG: {
			// populate the controls
		//	Button_SetCheck(GetDlgItem(hwndDlg, IDCFULLSCREEN), fullscreen ? BST_CHECKED : BST_UNCHECKED);
		//	PopulateVideoModeLists(fullscreen, GetDlgItem(hwndDlg, IDC2DVMODE), GetDlgItem(hwndDlg, IDC3DVMODE));
		//	return TRUE;
		//}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCFULLSCREEN:
					fullscreen = Button_GetCheck((HWND)lParam) == BST_CHECKED ? 1:0;
					PopulateVideoModeLists(fullscreen, GetDlgItem(hwndDlg, IDC2DVMODE),
							GetDlgItem(hwndDlg, IDC3DVMODE));
					break;
				case IDC2DVMODE:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						int i;
						i = ComboBox_GetCurSel((HWND)lParam);
						if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
						if (i != CB_ERR) {
							xdim2d = validmode[i].xdim;
							ydim2d = validmode[i].ydim;
						}
					}
					break;
				case IDC3DVMODE:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						int i;
						i = ComboBox_GetCurSel((HWND)lParam);
						if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
						if (i != CB_ERR) {
							xdimgame = validmode[i].xdim;
							ydimgame = validmode[i].ydim;
							bppgame  = validmode[i].bpp;
						}
					}
					break;
				default: break;
			}
			break;
		default: break;
	}
	return FALSE;
}


static HWND startupdlg = NULL;
static HWND pages[2] = { NULL, NULL};
static int done = -1;


static void SetPage(int n)
{
	HWND tab;
	int cur;
	tab = GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL);
	cur = (int)SendMessage(tab, TCM_GETCURSEL,0,0);
	ShowWindow(pages[cur],SW_HIDE);
	SendMessage(tab, TCM_SETCURSEL, n, 0);
	ShowWindow(pages[n],SW_SHOWDEFAULT);
}

static void EnableConfig(int n)
{
	EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_CANCEL), n);
	EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_START), n);
	EnableWindow(GetDlgItem(pages[0], IDCFULLSCREEN), n);
	EnableWindow(GetDlgItem(pages[0], IDC2DVMODE), n);
	EnableWindow(GetDlgItem(pages[0], IDC3DVMODE), n);
}

static INT_PTR CALLBACK startup_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBITMAP hbmp = NULL;
	HDC hdc;
	
	switch (uMsg) {
		case WM_INITDIALOG: {
			HWND hwnd;
			RECT r, rdlg, chrome, rtab, rcancel, rstart;
			int xoffset = 0, yoffset = 0;

			// Fetch the positions (in screen coordinates) of all the windows we need to tweak
			ZeroMemory(&chrome, sizeof(chrome));
			AdjustWindowRect(&chrome, GetWindowLong(hwndDlg, GWL_STYLE), FALSE);
			GetWindowRect(hwndDlg, &rdlg);
			GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL), &rtab);
			GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_CANCEL), &rcancel);
			GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_START), &rstart);

			// Knock off the non-client area of the main dialogue to give just the client area
			rdlg.left -= chrome.left; rdlg.top -= chrome.top;
			rdlg.right -= chrome.right; rdlg.bottom -= chrome.bottom;

			// Translate them to client-relative coordinates wrt the main dialogue window
			rtab.right -= rtab.left - 1; rtab.bottom -= rtab.top - 1;
			rtab.left  -= rdlg.left; rtab.top -= rdlg.top;

			rcancel.right -= rcancel.left - 1; rcancel.bottom -= rcancel.top - 1;
			rcancel.left -= rdlg.left; rcancel.top -= rdlg.top;

			rstart.right -= rstart.left - 1; rstart.bottom -= rstart.top - 1;
			rstart.left -= rdlg.left; rstart.top -= rdlg.top;

			// And then convert the main dialogue coordinates to just width/length
			rdlg.right -= rdlg.left - 1; rdlg.bottom -= rdlg.top - 1;
			rdlg.left = 0; rdlg.top = 0;

			// Load the bitmap into the bitmap control and fetch its dimensions
			hbmp = LoadBitmap((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(RSRC_BMP));
			hwnd = GetDlgItem(hwndDlg,WIN_STARTWIN_BITMAP);
			SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
			GetClientRect(hwnd, &r);
			xoffset = r.right;
			yoffset = r.bottom - rdlg.bottom;

			// Shift and resize the controls that require it
			rtab.left += xoffset; rtab.bottom += yoffset;
			rcancel.left += xoffset; rcancel.top += yoffset;
			rstart.left += xoffset; rstart.top += yoffset;
			rdlg.right += xoffset;
			rdlg.bottom += yoffset;

			// Move the controls to their new positions
			MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL), rtab.left, rtab.top, rtab.right, rtab.bottom, FALSE);
			MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_CANCEL), rcancel.left, rcancel.top, rcancel.right, rcancel.bottom, FALSE);
			MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_START), rstart.left, rstart.top, rstart.right, rstart.bottom, FALSE);

			// Move the main dialogue to the centre of the screen
			hdc = GetDC(NULL);
			rdlg.left = (GetDeviceCaps(hdc, HORZRES) - rdlg.right) / 2;
			rdlg.top = (GetDeviceCaps(hdc, VERTRES) - rdlg.bottom) / 2;
			ReleaseDC(NULL, hdc);
			MoveWindow(hwndDlg, rdlg.left + chrome.left, rdlg.top + chrome.left,
				rdlg.right + (-chrome.left+chrome.right), rdlg.bottom + (-chrome.top+chrome.bottom), TRUE);

			// Add tabs to the tab control
			{
				TCITEM tab;
				
				hwnd = GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL);

				ZeroMemory(&tab, sizeof(tab));
				tab.mask = TCIF_TEXT;
				tab.pszText = TEXT("Configuration");
				SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)0, (LPARAM)&tab);
				tab.mask = TCIF_TEXT;
				tab.pszText = TEXT("Messages");
				SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)1, (LPARAM)&tab);

				// Work out the position and size of the area inside the tab control for the pages
				ZeroMemory(&r, sizeof(r));
				GetClientRect(hwnd, &r);
				SendMessage(hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&r);
				r.right -= r.left-1;
				r.bottom -= r.top-1;
				r.top += rtab.top;
				r.left += rtab.left;

				// Create the pages and position them in the tab control, but hide them
				pages[0] = CreateDialog((HINSTANCE)win_gethinstance(),
					MAKEINTRESOURCE(WIN_STARTWINPAGE_CONFIG), hwndDlg, ConfigPageProc);
				pages[1] = GetDlgItem(hwndDlg, WIN_STARTWIN_MESSAGES);
				SetWindowPos(pages[0], hwnd,r.left,r.top,r.right,r.bottom,SWP_HIDEWINDOW);
				SetWindowPos(pages[1], hwnd,r.left,r.top,r.right,r.bottom,SWP_HIDEWINDOW);

				// Tell the editfield acting as the console to exclude the width of the scrollbar
				GetClientRect(pages[1],&r);
				r.right -= GetSystemMetrics(SM_CXVSCROLL)+4;
				r.left = r.top = 0;
				SendMessage(pages[1], EM_SETRECTNP,0,(LPARAM)&r);
			}
			return TRUE;
		}

		case WM_NOTIFY: {
			LPNMHDR nmhdr = (LPNMHDR)lParam;
			int cur;
			if (nmhdr->idFrom != WIN_STARTWIN_TABCTL) break;
			cur = (int)SendMessage(nmhdr->hwndFrom, TCM_GETCURSEL,0,0);
			switch (nmhdr->code) {
				case TCN_SELCHANGING: {
					if (cur < 0 || !pages[cur]) break;
					ShowWindow(pages[cur],SW_HIDE);
					return TRUE;
				}
				case TCN_SELCHANGE: {
					if (cur < 0 || !pages[cur]) break;
					ShowWindow(pages[cur],SW_SHOWDEFAULT);
					return TRUE;
				}
			}
			break;
		}

		case WM_CLOSE:
			done = 0;
			return TRUE;

		case WM_DESTROY:
			if (hbmp) {
				DeleteObject(hbmp);
				hbmp = NULL;
			}

			if (pages[0]) {
				DestroyWindow(pages[0]);
				pages[0] = NULL;
			}

			startupdlg = NULL;
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case WIN_STARTWIN_CANCEL: done = 0; return TRUE;
				case WIN_STARTWIN_START: done = 1; return TRUE;
			}
			return FALSE;

		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == pages[1])
				return (BOOL)GetSysColorBrush(COLOR_WINDOW);
			break;

		default: break;
	}

	return FALSE;
}


int startwin_open(void)
{
	INITCOMMONCONTROLSEX icc;
	if (startupdlg) return 1;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icc);
	startupdlg = CreateDialog((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(WIN_STARTWIN), NULL, startup_dlgproc);
	if (startupdlg) {
		SetPage(1);
		EnableConfig(0);
		return 0;
	}
	return -1;
}

int startwin_close(void)
{
	if (!startupdlg) return 1;
	DestroyWindow(startupdlg);
	startupdlg = NULL;
	return 0;
}

int startwin_puts(const char *buf)
{
	const char *p = NULL, *q = NULL;
	char workbuf[1024];
	static int newline = 0;
	int curlen, linesbefore, linesafter;
	HWND edctl;

	if (!startupdlg) return 1;
	
	edctl = pages[1];
	if (!edctl) return -1;

	SendMessage(edctl, WM_SETREDRAW, FALSE,0);
	curlen = SendMessage(edctl, WM_GETTEXTLENGTH, 0,0);
	SendMessage(edctl, EM_SETSEL, (WPARAM)curlen, (LPARAM)curlen);
	linesbefore = SendMessage(edctl, EM_GETLINECOUNT, 0,0);
	p = buf;
	while (*p) {
		if (newline) {
			SendMessage(edctl, EM_REPLACESEL, 0, (LPARAM)"\r\n");
			newline = 0;
		}
		q = p;
		while (*q && *q != '\n') q++;
		memcpy(workbuf, p, q-p);
		if (*q == '\n') {
			if (!q[1]) {
				newline = 1;
				workbuf[q-p] = 0;
			} else {
				workbuf[q-p] = '\r';
				workbuf[q-p+1] = '\n';
				workbuf[q-p+2] = 0;
			}
			p = q+1;
		} else {
			workbuf[q-p] = 0;
			p = q;
		}
		SendMessage(edctl, EM_REPLACESEL, 0, (LPARAM)workbuf);
	}
	linesafter = SendMessage(edctl, EM_GETLINECOUNT, 0,0);
	SendMessage(edctl, EM_LINESCROLL, 0, linesafter-linesbefore);
	SendMessage(edctl, WM_SETREDRAW, TRUE,0);
	return 0;
}

int startwin_settitle(const char *str)
{
	if (!startupdlg) return 1;
	SetWindowText(startupdlg, str);
	return 0;
}

int startwin_idle(void *v)
{
	if (!startupdlg || !IsWindow(startupdlg)) return 0;
	if (IsDialogMessage(startupdlg, (MSG*)v)) return 1;
	//if (IsDialogMessage(pages[0], (MSG*)v)) return 1;
	return 0;
}

int startwin_run(void)
{
	MSG msg;
	if (!startupdlg) return 1;

	done = -1;

	SetPage(0);
	EnableConfig(1);

	while (done < 0) {
		switch (GetMessage(&msg, NULL, 0,0)) {
			case 0: done = 1; break;
			case -1: return -1;
			default:
				 if (IsWindow(startupdlg) && IsDialogMessage(startupdlg, &msg)) break;
				 TranslateMessage(&msg);
				 DispatchMessage(&msg);
				 break;
		}
	}

	SetPage(1);
	EnableConfig(0);

	return done;
}

