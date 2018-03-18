// FileExplorer.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "FileExplorer.h"
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <Commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib,"Shlwapi.lib")


#define MAX_LOADSTRING 100
#define ADDRESS_HEIGHT 30
#define STATUS_HEIGHT 32
#define LINE 2

// Global Variables:
static HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND g_hwnd;
static HACCEL hAccelTable;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
POINT GetStartPoint(int, int);
void GetInfoPC();//Lấy thông tin PC
void LoadTree(HWND);//Load Tree
void LoadExpand(HWND, HTREEITEM);//Load con khi bấm +
int CheckChild(LPSHELLFOLDER, LPITEMIDLIST);//Kiểm tra con
void LoadListPC(HWND);//Load listview của this PC
void LoadList (HWND, LPCWSTR);//Load listview của đường dẫn
void StatusBarItem();//Số item trong listview
LPWSTR ConvertSize(unsigned long long size);//byte sang dạng đơn vị khác
void ColForFolder(HWND);//Tạo cột cho khung folder
void ColForDrive(HWND);//Tạo cột cho khung driver
LPWSTR GetDateModified(const FILETIME&);//Lấy ngày thay đổi
LPCWSTR GetPath(HWND , HTREEITEM);
int ItemEnter(HWND,LPCWSTR);//Truy cập vào item
LPCWSTR GetPath(HWND hWndListView, int iItem);
LPCWSTR Return(LPCWSTR);//Trở về trước 
LPWSTR GetTypeFile(LPWSTR name);//Lấy loại file dựa vào đuôi
WCHAR* GetType(WIN32_FIND_DATA fd);//Lấy loại tập tin có mô tả
void ChangeView(LONG style);//Thay đổi cách liệt kê
void LoadIcon();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FILEEXPLORER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILEEXPLORER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(g_hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
    return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FILEEXPLORER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(RGB(255,255,255));
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_FILEEXPLORER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);

}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   POINT center = GetStartPoint(1000, 600);

   HWND hWnd = CreateWindow(szWindowClass, L"File Explorer", WS_OVERLAPPEDWINDOW,
      center.x, center.y, 1000, 600, nullptr, nullptr, hInstance, nullptr);

   g_hwnd = hWnd;

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

struct Drive
{
	 WCHAR driver[26][4] = { 0 };
	 WCHAR drivetype[26][30] = { 0 };
	 WCHAR displayname[26][1024] = { 0 };
	 int count = 0;
};

static const int BUFFERSIZE = 260;
static WCHAR curPath[BUFFERSIZE];
static WCHAR configPath[BUFFERSIZE];
static WCHAR buffer[10];
static WCHAR address[2048] = L"This PC";
static WCHAR adrCopy[2048] = { 0 };
static WCHAR nameCopy[1024] = { 0 };
static HWND hWndTreeView;
static HWND hWndListView;
static HWND lblAddress;
static HWND hWndStatusBar;
static Drive dr;
static LOGFONT lf;
static HICON s;
static HFONT font;
static ACCEL * backup;
static HIMAGELIST hmlbig;
static HIMAGELIST hmlsmall;
static LV_COLUMN lvCol;
static WORD t;//Biến của icon
static int tvWidth;
static int preWidth;
static int height;
static int width;
static bool xSizing;
static int bigIcon;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Tạo đường dẫn tuyệt đối tới file config
	switch (message)
	{
	case WM_SIZE:
	{
		height = HIWORD(lParam);
		width = LOWORD(lParam);
		RECT rect;
		GetWindowRect(hWnd, &rect);
		MoveWindow(GetDlgItem(hWnd, IDB_BACK), 2 , 0, 30, 30, 1);
		MoveWindow(lblAddress, 32, 2, width - 32, 28, 1);
		SendMessage(hWndStatusBar, SB_SETMINHEIGHT, STATUS_HEIGHT, 0);
		MoveWindow(hWndStatusBar, 0, 0, 0, 0, 1);

		if (height < 200)
		{
			height = 200;
			MoveWindow(hWnd, rect.left, rect.top, rect.right - rect.left, 200, 1);
		}
		if (width - tvWidth > 200)
		{
			MoveWindow(hWndTreeView, 0, ADDRESS_HEIGHT, tvWidth, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
			MoveWindow(hWndListView, tvWidth + 2, ADDRESS_HEIGHT, width - tvWidth - 2, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
			preWidth = tvWidth;
		}
		else
		{
			MoveWindow(hWndListView, width - 200 + 2, ADDRESS_HEIGHT, 202, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
			MoveWindow(hWndTreeView, 0, ADDRESS_HEIGHT, width - 200, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
			preWidth = width - 200;
		}
	}
	break;
	case WM_MOUSEMOVE:
	{
		int xPos;
		int yPos;
		xPos = (int)LOWORD(lParam);
		yPos = (int)HIWORD(lParam);

		if((xPos > preWidth - 5) && (xPos < preWidth + 5) && yPos > 30)
			SetCursor(LoadCursor(nullptr, MAKEINTRESOURCE(32644)));
		else
			SetCursor(LoadCursor(nullptr, IDC_ARROW));

		if (wParam == MK_LBUTTON)
		{
			if (xSizing)
			{
				SetCursor(LoadCursor(nullptr, MAKEINTRESOURCE(32644)));
				if (width - xPos > 200 && xPos > 100)
				{
					preWidth = xPos;
					MoveWindow(hWndTreeView, 0, ADDRESS_HEIGHT, preWidth, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
					MoveWindow(hWndListView, preWidth + 2, ADDRESS_HEIGHT, width - preWidth - 2, height - ADDRESS_HEIGHT - STATUS_HEIGHT - 3, 1);
					tvWidth = preWidth;
				}
			}
		}
	}
	break;
	case WM_LBUTTONDOWN:
	{
		int xPos;
		int yPos;
		// Varible used to get the mouse cursor x and y co-ordinates
		xPos = (int)LOWORD(lParam);
		yPos = (int)HIWORD(lParam);
		if ((xPos > preWidth - 5) && (xPos < preWidth + 5) && yPos > 30)
		{
			xSizing = 1;
			SetCapture(hWnd);
			if (xSizing)
			{
				SetCursor(LoadCursor(nullptr, MAKEINTRESOURCE(32644)));
			}
		}
	}
	break;
	case WM_LBUTTONUP:
	{
		xSizing = 0;
		ReleaseCapture();
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
	}
	break;
	case WM_CREATE:
	{
		GetCurrentDirectory(BUFFERSIZE, curPath);
		wsprintf(configPath, L"%s\\config.ini", curPath);
		GetPrivateProfileString(L"app", L"height", L"600", buffer, 10, configPath);
		height = _wtoi(buffer);
		GetPrivateProfileString(L"app", L"width", L"1000", buffer, 10, configPath);
		width = _wtoi(buffer);
		GetPrivateProfileString(L"app", L"tvWidth", L"300", buffer, 10, configPath);
		tvWidth = _wtoi(buffer);
		//Kích thước cửa sổ
		MoveWindow(hWnd, GetStartPoint(width, height).x, GetStartPoint(width, height).y, width, height, 1);

		//Set font 
		lf.lfHeight = 18;
		lf.lfWidth = 7;
		wcscpy_s(lf.lfFaceName, L"Consolas");
		font = CreateFontIndirect(&lf);

		CreateWindow(L"button", L"", WS_CHILD | WS_VISIBLE | BS_FLAT |BS_ICON | BS_PUSHBUTTON, 270 , 0, 30, 30, hWnd, (HMENU)IDB_BACK, hInst, NULL);
		s = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON9));//Button back
		SendMessage(GetDlgItem(hWnd, IDB_BACK), BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)s);

		lblAddress = CreateWindowEx(WS_EX_CLIENTEDGE,L"static", L"This PC", WS_VISIBLE | WS_CHILD  |SS_LEFT, 0, 0 , 0, 0, hWnd, NULL, hInst, 0);
		SendMessage(lblAddress, WM_SETFONT, WPARAM(font), TRUE);

		hWndTreeView = CreateWindowEx(WS_EX_CLIENTEDGE,WC_TREEVIEW, L"", WS_CHILD | WS_VISIBLE  | TVS_HASLINES | TVS_HASBUTTONS | WS_VSCROLL | WS_HSCROLL | TVS_LINESATROOT, 0, 0, 0, 0, hWnd, (HMENU)IDM_TREEVIEW, hInst, NULL);

		hWndListView = CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | LVS_REPORT, 0, 0, 0, 0, hWnd, (HMENU)IDM_LISTVIEW, hInst, 0);
		ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);

		hWndStatusBar = CreateWindowEx(WS_EX_CLIENTEDGE,STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, 0, hInst, 0);
		int sizes[3] = { 200,400,-1};
		SendMessage(hWndStatusBar, SB_SETPARTS, 3, (LPARAM)&sizes);
		SendMessage(hWndStatusBar, SB_SETMINHEIGHT, 32, 0);

		/*Load icon*/
		LoadIcon();
		SendMessage(hWndTreeView, TVM_SETIMAGELIST, 0, (LPARAM)hmlsmall);
		ListView_SetImageList(hWndListView, hmlsmall, LVSIL_SMALL);
		ListView_SetImageList(hWndListView, hmlbig, LVSIL_NORMAL);

		/*Get info drivers*/
		GetInfoPC();
		/*TREE VIEW*/
		LoadTree(hWndTreeView);
		/*LIST VIEW*/
		LoadListPC(hWndListView);
		SetFocus(hWndListView);

	}
		break;
	case WM_CONTEXTMENU:
	{
		int xPos = LOWORD(lParam);
		int yPos = HIWORD(lParam);
		HMENU hMenu = CreatePopupMenu();
		InsertMenu(hMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_COPY, L"Copy");
		InsertMenu(hMenu, 1, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_PASTE, L"Paste");
		InsertMenu(hMenu, 2, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SELECTALL, L"Selectall");
		InsertMenu(hMenu, 3, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_DELETE, L"Delete");

		TrackPopupMenu(hMenu, TPM_LEFTBUTTON| TPM_TOPALIGN,xPos,yPos,0,hWnd,NULL);	
	}
		break;
	case WM_NOTIFY:
	{
		//lParam chứa đối tượng và mã sự kiện
		UINT code = ((LPNMHDR)lParam)->code;
		switch (code)
		{
		case TVN_ITEMEXPANDING://Sự kiện bấm dấu +
			LoadExpand(hWndTreeView, ((LPNMTREEVIEW)(LPNMHDR)lParam)->itemNew.hItem);
			break;
		case TVN_SELCHANGED://Sự kiện chọn item tree view
		{
			ListView_DeleteAllItems(hWndListView); //Xóa sạch List View
			LPCWSTR m_path = (LPCWSTR)((LPNMTREEVIEW)(LPNMHDR)lParam)->itemNew.lParam;
			LoadList(hWndListView, m_path);
			wcscpy_s(address, 2048, m_path);
			SendMessage(lblAddress, WM_SETTEXT, 0, (LPARAM)address);
			break;
		}
		case LVN_ITEMCHANGED:
		{
			WCHAR numitem[20];
			int num = ListView_GetSelectedCount(hWndListView);
			_itow_s(num, numitem, 10);
			if (num == 0)
			{
				SendMessage(hWndStatusBar, SB_SETTEXT, 1, 0);
				break;
			}
			else if (num > 1)
				wcscat_s(numitem, L" items selected");
			else
				wcscat_s(numitem, L" item selected");
			SendMessage(hWndStatusBar, SB_SETTEXT, 1, (LPARAM)numitem);
		}
			break;
		case NM_CLICK:
		{
			SendMessage(hWndStatusBar, SB_SETTEXT, 2, 0);
			if ((((LPNMHDR)lParam)->hwndFrom) != hWndListView)
			{
				for (int i = ListView_GetItemCount(hWndListView) - 1; i >= 0; --i)
				{
					ListView_SetItemState(hWndListView, i, 0,LVIS_SELECTED);
					ListView_SetSelectionMark(hWndListView, i);
				}
			}
			if ((((LPNMHDR)lParam)->hwndFrom) == hWndListView)
			{
				if (((LPNMITEMACTIVATE)lParam)->iItem == -1)
					ListView_SetSelectionMark(hWndListView, -1);
				else
				{
					LPCWSTR m_path = GetPath(hWndListView, ((LPNMITEMACTIVATE)lParam)->iItem);
					WIN32_FIND_DATA fd;
					GetFileAttributesEx(m_path, GetFileExInfoStandard, &fd);
					if (((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
					{
						SendMessage(hWndStatusBar, SB_SETTEXT, 2, (LPARAM)ConvertSize(fd.nFileSizeLow));
					}
					SendMessage(hWndStatusBar, SB_SETICON, 0, (LPARAM)ExtractAssociatedIconW(hInst, (LPWSTR)m_path, &t));

				}
			}
		}
		break;
		case NM_DBLCLK://Sự kiện doubleclick
			if ((((LPNMHDR)lParam)->hwndFrom) == hWndListView)
			{
				if (((LPNMITEMACTIVATE)lParam)->iItem != -1)
				{
					LPCWSTR m_path = GetPath(hWndListView, ((LPNMITEMACTIVATE)lParam)->iItem);
					if(!ItemEnter(hWndListView, m_path))//Nếu file thực thi sẽ trả về 0
						wcscpy_s(address, 2048, m_path);
					SendMessage(lblAddress, WM_SETTEXT, 0, (LPARAM)address);
				}
			}
			break;
		}
	}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
			switch (wmId)
			{
			case IDB_BACK:
				ListView_DeleteAllItems(hWndListView);
				LoadList(hWndListView, Return(address));
				SendMessage(lblAddress, WM_SETTEXT, 0, (LPARAM)address);
				break;
			case ID_ENTER:
				if (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) != -1)
				{
					LPCWSTR m_path = GetPath(hWndListView, ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED));
					if (!ItemEnter(hWndListView, m_path))
						wcscpy_s(address, 2048, m_path);
					SendMessage(lblAddress, WM_SETTEXT, 0, (LPARAM)address);
				}
				break;
			case ID_UP:
				SendMessage(hWnd, WM_COMMAND, IDB_BACK, 0);
				break;
			case ID_RELOAD:
				dr.count = 0;
				GetInfoPC();
				TreeView_DeleteAllItems(hWndTreeView);
				LoadTree(hWndTreeView);
				ListView_DeleteAllItems(hWndListView);
				LoadList(hWndListView, address);
				SetFocus(hWndListView);
				break;
			case ID_VIEW_LARGE:
				ChangeView(LVS_ICON);
				break;
			case ID_VIEW_SMALL:
				ChangeView(LVS_SMALLICON);
				break;
			case ID_VIEW_LIST:
				ChangeView(LVS_LIST);
				break;
			case ID_VIEW_REPORT:
				ChangeView(LVS_REPORT);
				break;
			case IDM_ABOUT:
				MessageBox(hWnd, L"Nguyễn Đình Tiến - 1512569", L"Author", 0);
				break;
			case ID_COPY:
			{
				if (ListView_GetSelectionMark(hWndListView) != -1)
				{
					WCHAR m_path[2048];
					wcscpy_s(m_path, GetPath(hWndListView, ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED)));
					ListView_GetItemText(hWndListView, ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED), 0, nameCopy, 1024);
					memset(adrCopy, 0, 2048);
					wcscpy_s(adrCopy, 2048, m_path);
				}
				break;
			}
			case ID_PASTE:
			{
				WCHAR m_path[2048] = { 0 };
				//chuột phải item
				if (ListView_GetSelectionMark(hWndListView) != -1)
				{
					wcscpy_s(m_path, GetPath(hWndListView, ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED)));
				}
				//chuột phải ngoài item
				else
				{
					//Nếu ở This PC
					if (!wcscmp(address, L"This PC"))
						break;
					wcscpy_s(m_path, address);
				}
				//Copy vào đích thư mục hoặc ổ đĩa
				if ((GetFileAttributes(m_path) & FILE_ATTRIBUTE_DIRECTORY) || ((GetFileAttributes(m_path) & FILE_ATTRIBUTE_DEVICE)))
				{
					//Gốc là thư mục thì copy nội dung trong nó rồi nối tên nó vào đích
					if ((GetFileAttributes(adrCopy) & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (wcslen(m_path) != 3)
							wcscat_s(m_path, L"\\");
						wcscat_s(m_path, nameCopy);//nối tên thư mục vào đích
						if (!wcscmp(m_path, adrCopy))
							wcscat_s(m_path, L"COPY");

						wcscat_s(adrCopy, L"\\*");
						adrCopy[wcslen(adrCopy) + 1] = 0;//Xong nguồn
					}
					//còn là tập tin
					else
					{
						adrCopy[wcslen(adrCopy) + 1] = 0;//Xong nguồn
					}

					m_path[wcslen(m_path) + 1] = 0;//Xong đích


					SHFILEOPSTRUCT sh;
					sh.hwnd = NULL;
					sh.pFrom = adrCopy;
					sh.pTo = m_path;
					sh.lpszProgressTitle = L"Copy";
					sh.wFunc = FO_COPY;
					sh.fFlags = FOF_RENAMEONCOLLISION | FOF_SIMPLEPROGRESS | FOF_NOCONFIRMMKDIR;
					if (!SHFileOperation(&sh))
					{
						MessageBeep(0);
					}
					SendMessage(hWnd, WM_COMMAND, ID_RELOAD, 0);
				}
			}
			break;
			case ID_SELECTALL:
			{
				for (int i = ListView_GetItemCount(hWndListView) - 1; i >= 0; --i)
				{
					ListView_SetItemState(hWndListView, i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				}
			}
				break;
			case ID_DELETE:
			{
				WCHAR m_path[2048];
				SHFILEOPSTRUCT sh;
				sh.hwnd = NULL;
				sh.lpszProgressTitle = L"Delete";
				sh.wFunc = FO_DELETE;
				sh.fFlags = FOF_ALLOWUNDO | FOF_WANTNUKEWARNING | FOF_SIMPLEPROGRESS;
				int hoi = MessageBox(hWnd, L"Cho vào sọt rác nhé?", L"Ơ KÌA !", MB_YESNOCANCEL | MB_ICONQUESTION);
				if (hoi == IDNO)
					sh.fFlags = (~FOF_ALLOWUNDO) & sh.fFlags;
				else if (hoi == IDCANCEL)
					break;
				int iPos = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
				while (iPos != -1) {
					wcscpy_s(m_path, GetPath(hWndListView, iPos));
					m_path[wcslen(m_path) + 1] = 0;
					sh.pFrom = m_path;
					SHFileOperation(&sh);
					iPos = ListView_GetNextItem(hWndListView, iPos, LVNI_SELECTED);
				}
				SendMessage(hWnd, WM_COMMAND, ID_RELOAD, 0);
			}
				break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_DESTROY:
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);
		_itow_s(rect.bottom - rect.top, buffer, 10);
		WritePrivateProfileString(L"app", L"height", buffer, configPath);
		_itow_s(rect.right - rect.left, buffer, 10);
		WritePrivateProfileString(L"app", L"width", buffer, configPath);
		_itow_s(tvWidth, buffer, 10);
		WritePrivateProfileString(L"app", L"tvWidth", buffer, configPath);
		PostQuitMessage(0);
	}
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
//Lấy đường dẫn của đối tượng hItem
LPCWSTR GetPath(HWND hWndTV,HTREEITEM hItem)
{
	TVITEM tv;
	tv.mask = TVIF_PARAM;
	tv.hItem = hItem;
	TreeView_GetItem(hWndTV, &tv);
	return (LPCWSTR)tv.lParam;
}
//Load các item con của item tree hParent
void LoadExpand(HWND hWndTreeView, HTREEITEM hParent)
{
	HTREEITEM child = TreeView_GetChild(hWndTreeView, hParent);
	if (child != NULL)
		return;
	WCHAR path[1024];
	wcscpy_s(path, GetPath(hWndTreeView, hParent));
	if (path == NULL)
		return;
	LPSHELLFOLDER psFolder = NULL;
	LPENUMIDLIST penumID = NULL;
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = NULL;
	LPITEMIDLIST itembandau = ILCreateFromPath(path);

	SHBindToObject(NULL, itembandau, NULL, IID_IShellFolder, (void**)&psFolder);
	psFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &penumID);
	do
	{
		hr = penumID->Next(1, &pidl, NULL);
		if (hr != S_OK)
			break;
		WCHAR chuoitenfile[1000];
		STRRET xuat;
		psFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &xuat);
		StrRetToBuf(&xuat, pidl, chuoitenfile, 1000);
		WCHAR * buffer = new WCHAR[wcslen(path) + wcslen(chuoitenfile) + 2];
		wcscpy_s(buffer, wcslen(path) + wcslen(chuoitenfile) + 2,path);
		if(wcslen(buffer) != 3)
			wcscat_s(buffer, wcslen(path) + wcslen(chuoitenfile) + 2,L"\\");
		wcscat_s(buffer, wcslen(path) + wcslen(chuoitenfile) + 2,chuoitenfile);
		TV_INSERTSTRUCT tv;
		tv.hParent = hParent;
		tv.hInsertAfter = TVI_LAST;
		tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM |TVIF_CHILDREN;
		tv.item.iImage = 1;
		tv.item.iSelectedImage = 1;
		tv.item.pszText = chuoitenfile;
		tv.item.lParam = (LPARAM)buffer;
		tv.item.cChildren = 0;
		if (CheckChild(psFolder, pidl))
			tv.item.cChildren = 1;
		HTREEITEM itnew = TreeView_InsertItem(hWndTreeView, &tv);
	} while (hr == S_OK);	
	psFolder->Release();
	penumID->Release();
}
//Điểm để canh cửa sổ chính giữa
POINT GetStartPoint(int x, int y)
{
	POINT center;
	RECT rect;
	HWND desktop = GetDesktopWindow();
	GetWindowRect(desktop, &rect);
	center.x = (rect.right - x) / 2;
	center.y = (rect.bottom - y) / 2;
	return center;
}
//Đổi size dạng byte sang dung lượng có đơn vị
LPWSTR ConvertSize(unsigned long long size)
{
	if (size == 0)
		return L"";

	int type = 0;
	double dsize = (double)size;
	while (dsize >= 1024)
	{
		dsize /= 1024;
		type++;
	}
	if (dsize == 0)
		dsize = 1;
	WCHAR * res = new WCHAR[15];
	if (dsize == 0)
		return L"";
	if(type > 1)
		swprintf_s(res,15, L"%.1f", dsize);
	else
		swprintf_s(res, 15, L"%.0f", dsize);

	switch (type)
	{
	case 0://Bytes
		wcscat_s(res,15, _T(" bytes"));
		break;
	case 1:
		wcscat_s(res,15,_T(" KB"));
		break;
	case 2:
		wcscat_s(res,15,_T(" MB"));
		break;
	case 3:
		wcscat_s(res,15, _T(" GB"));
		break;
	wcscat_s(res,15, _T(" TB"));
	}
	return res;
}
//Column cho thư mục và tập tin
void ColForFolder(HWND hWndListView)
{
	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 200;
	lvCol.pszText = _T("Tên");
	ListView_InsertColumn(hWndListView,0,&lvCol);

	lvCol.cx = 80;
	lvCol.fmt = LVCFMT_RIGHT;
	lvCol.pszText = _T("Kích thước");
	ListView_InsertColumn(hWndListView, 1, &lvCol);

	lvCol.cx = 220;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.pszText = _T("Loại");
	ListView_InsertColumn(hWndListView, 2, &lvCol);


	lvCol.cx = 130;
	lvCol.pszText = _T("Thời gian chỉnh sửa");
	ListView_InsertColumn(hWndListView, 3, &lvCol);

}
//Tạo column cho ổ đĩa
void ColForDrive(HWND hWndListView)
{

	lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 120;
	lvCol.pszText = _T("Tên");
	ListView_InsertColumn(hWndListView, 0, &lvCol);

	lvCol.cx = 120;
	lvCol.pszText = _T("Kích thước");
	ListView_InsertColumn(hWndListView, 2, &lvCol);

	lvCol.cx = 120;
	lvCol.pszText = _T("Loại");
	ListView_InsertColumn(hWndListView, 1, &lvCol);


	lvCol.cx = 125;
	lvCol.pszText = _T("Dung lượng trống");
	ListView_InsertColumn(hWndListView, 3, &lvCol);

}
//Lấy thông tin PC
void GetInfoPC()
{
	static WCHAR buffer[1024];

	GetLogicalDriveStrings(1024, buffer);

	for (int i = 0;!(buffer[i] == 0 && buffer[i + 1] == 0);i++)
		if (buffer[i] == 0)
		{
			dr.count++;
		}
	dr.count++;

	int m = 0;;
	for (int i = 0; m < dr.count; i += 4)
	{
		wcscpy_s(dr.driver[m], &buffer[i]);
		m++;
	}

	for (int i = 0; i < dr.count; i++)
	{

		int type = GetDriveType(dr.driver[i]);
		if (type == DRIVE_FIXED)
		{
			wcscpy_s(dr.drivetype[i], L"Local disk");
			GetVolumeInformation(dr.driver[i], buffer, 1024, NULL, NULL, NULL, NULL, 0);
			wcscpy_s(dr.displayname[i], buffer);
			if (buffer[0] == 0)
				wcscat_s(dr.displayname[i], L"Local disk");
		}
		else if (type == DRIVE_CDROM)
		{
			wcscpy_s(dr.displayname[i], L"CD Drive");
			wcscpy_s(dr.drivetype[i], L"CD Drive");
		}
		else if (i > 1 && type == DRIVE_REMOVABLE)
		{
			GetVolumeInformation(dr.driver[i], buffer, 1024, NULL, NULL, NULL, NULL, 0);
			wcscpy_s(dr.displayname[i], buffer);
			if (buffer[0] == 0)
				wcscat_s(dr.displayname[i], L"USB");
			wcscpy_s(dr.drivetype[i], L"Removeble disk");

		}
		wcscat_s(dr.displayname[i], L" (");
		wcsncat_s(dr.displayname[i], dr.driver[i], 1);
		wcscat_s(dr.displayname[i], L":)");
	}

}
//Load treeview
void LoadTree(HWND hWndTreeView)
{
	static WCHAR buffer[1024];
	HTREEITEM it;
	TV_INSERTSTRUCT tv;
	tv.hParent = TVI_ROOT;
	tv.hInsertAfter = TVI_LAST;
	tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tv.item.iImage = 0;
	tv.item.iSelectedImage = 0;
	tv.item.pszText = L"This PC";
	tv.item.lParam = (LPARAM)L"This PC";
	it = TreeView_InsertItem(hWndTreeView, &tv);

	for (int i = 0; i < dr.count;i++)
	{
		int type = GetDriveType(dr.driver[i]);
		memset(buffer, 0, 1024);
		tv.hParent = it;
		tv.hInsertAfter = TVI_LAST;
		tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
		s = ExtractAssociatedIcon(hInst, dr.driver[i], &t);
		tv.item.iImage = ImageList_AddIcon(hmlsmall, s);
		tv.item.iSelectedImage = ImageList_AddIcon(hmlsmall, s);
		tv.item.pszText = dr.displayname[i];
		tv.item.lParam = (LPARAM)dr.driver[i];
		tv.item.cChildren = 0;
		if (type == DRIVE_FIXED || (type == DRIVE_REMOVABLE))
		{
			tv.item.cChildren = 1;
		}
		HTREEITEM it_child = TreeView_InsertItem(hWndTreeView, &tv);
	}
	TreeView_Expand(hWndTreeView, it, TVE_EXPAND);

}
//Load MyPC listview
void LoadListPC(HWND hWndListView)
{
	for (int i = 0; i < 4; i++)
		ListView_DeleteColumn(hWndListView, 0);

	ColForDrive(hWndListView);
	for (int i = 0; i < dr.count; i++)
	{
		LV_ITEM lv;
		lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lv.iItem = i;
		lv.iImage = ImageList_AddIcon(hmlsmall, ExtractAssociatedIconW(hInst, dr.driver[i], &t));
		if (bigIcon)
			lv.iImage = ImageList_AddIcon(hmlbig, ExtractAssociatedIconW(hInst, dr.driver[i], &t));
		lv.iSubItem = 0;//cột tên
		lv.pszText = dr.displayname[i];
		lv.lParam = (LPARAM)dr.driver[i];
		ListView_InsertItem(hWndListView, &lv);
		lv.mask = LVIF_TEXT;

		lv.iSubItem = 1;//Cột loại
		lv.pszText = dr.drivetype[i];
		ListView_SetItem(hWndListView, &lv);

		if (wcscmp(dr.drivetype[i], L"CD Drive"))//CD ROM
		{
			lv.iSubItem = 2;//cột kích thước
			unsigned long long size;
			SHGetDiskFreeSpaceEx(dr.driver[i], NULL, (PULARGE_INTEGER)&size, NULL);
			lv.pszText = ConvertSize(size);
			ListView_SetItem(hWndListView, &lv);

			lv.iSubItem = 3;//Dung lượng trống
			SHGetDiskFreeSpaceEx(dr.driver[i], NULL, NULL, (PULARGE_INTEGER)&size);
			lv.pszText = ConvertSize(size);
			ListView_SetItem(hWndListView, &lv);
		}
	}
	StatusBarItem();
}
//Load thư mục và tập tin vào list view
void LoadList(HWND hWndListView, LPCWSTR path)
{
	for (int i = 0; i < 4; i++)
		ListView_DeleteColumn(hWndListView, 0);
	if (path == NULL)
		return;
	if (wcscmp(path, L"This PC") == 0)
	{
		LoadListPC(hWndListView);
		return;
	}
	ColForFolder(hWndListView);
	WCHAR buffer[1024];
	wcscpy_s(buffer, path);

	WCHAR * folderPath;
	int count = 0;
	LPSHELLFOLDER psFolder = NULL;
	LPENUMIDLIST penumID = NULL;
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = NULL;
	LPITEMIDLIST itembandau = ILCreateFromPath(path);

	SHBindToObject(NULL, itembandau, NULL, IID_IShellFolder, (void**)&psFolder);
	if (itembandau == NULL)
		return;
	psFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penumID);
	if (penumID == NULL)
		return;
	do
	{
		hr = penumID->Next(1, &pidl, NULL);
		if (hr != S_OK)
			break;
		WCHAR chuoitenfile[1000];
		STRRET xuat;
		psFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &xuat);
		StrRetToBuf(&xuat, pidl, chuoitenfile, 1000);
		folderPath = new WCHAR[wcslen(path) + wcslen(chuoitenfile) + 2];
		wcscpy_s(folderPath, wcslen(path) + wcslen(chuoitenfile) + 2, path);

		if (wcslen(path) != 3)
			wcscat_s(folderPath, wcslen(path) + wcslen(chuoitenfile) + 2, L"\\");

		wcscat_s(folderPath, wcslen(path) + wcslen(chuoitenfile) + 2, chuoitenfile);
		LV_ITEM lv;
		lv.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lv.iItem = count;
		lv.iImage = ImageList_AddIcon(hmlsmall, ExtractAssociatedIconW(hInst, folderPath, &t));
		if(bigIcon)
			lv.iImage = ImageList_AddIcon(hmlbig, ExtractAssociatedIconW(hInst, folderPath, &t));
		lv.iSubItem = 0;		//Côt tên thư mục
		lv.pszText = chuoitenfile;
		lv.lParam = (LPARAM)folderPath;
		ListView_InsertItem(hWndListView, &lv);

		WIN32_FIND_DATA data;
		SHGetDataFromIDList(psFolder, pidl, SHGDFIL_FINDDATA, &data, sizeof(WIN32_FIND_DATA));
		ListView_SetItemText(hWndListView, count, 2, GetType(data));//Cột loại
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ListView_SetItemText(hWndListView, count, 2, L"THƯ MỤC");//Cột loại

		ListView_SetItemText(hWndListView, count, 1, ConvertSize(data.nFileSizeLow));//Cột kích thước

		ListView_SetItemText(hWndListView, count, 3, GetDateModified(data.ftLastWriteTime));//Cột Thời gian
		count++;
	} while (hr == S_OK);
	
	psFolder->Release();
	penumID->Release();
		
	StatusBarItem();//Thanh status bar
}
//Đổi thời gian sang chuỗi
LPWSTR GetDateModified(const FILETIME &ftLastWrite)
{
	SYSTEMTIME t;
	FileTimeToSystemTime(&ftLastWrite, &t);
	WCHAR *buffer = new WCHAR[50];
	swprintf_s(buffer,50,L"%d/%02d/%02d %02d:%02d %02s",
		t.wDay, t.wMonth, t.wYear, (t.wHour>12)?(t.wHour%12):t.wHour,
		t.wMinute, (t.wHour>12)?L"CH":L"SA");
	return buffer;
}
//Lấy đường dẫn từ thứ tự trong listview
LPCWSTR GetPath(HWND hWndListView,int iItem)
{
	LVITEM lv;
	lv.mask = LVIF_PARAM;
	lv.iItem = iItem;
	lv.iSubItem = 0;
	ListView_GetItem(hWndListView, &lv);
	return (LPCWSTR)lv.lParam;
}
//Click vào item
int ItemEnter(HWND hWndListView, LPCWSTR path)
{
	if (!path)
		return 0;
	WIN32_FIND_DATA fd;
	GetFileAttributesEx(path, GetFileExInfoStandard, &fd);
	if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		ListView_DeleteAllItems(hWndListView);
		LoadList(hWndListView, path);
		return 0;
	}
	ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
	return 1;
}
//Trở về thư mục trước
LPCWSTR Return(LPCWSTR path)
{
	if (wcscmp(path, L"This PC") == 0)
		return path;
	int len = wcslen(path);
	if (len <= 3)//ổ đĩa
	{
		swprintf_s((WCHAR*)path,8,L"This PC");
		return path;
	}
	int i;
	for (i = len - 1; i > 0 && path[i] != '\\';i--);
	if(i > 2)
		(WCHAR)path[i] = 0;
	else
		(WCHAR)path[i+1] = 0;
	return path;
}
//Lấy đuôi của tập tin
LPWSTR GetTypeFile(LPWSTR name)
{
	int i;
	int len = wcslen(name);
	WCHAR* res = new WCHAR[30];
	for (i = len - 1; i >= 0 && name[i] != '.'; i--);
	if (i == -1)
		return L"";
	wcscpy_s(res, 30, &name[i + 1]);
	_wcsupr_s(res, 30);
	return res;
}
//Lấy loại tập tin
WCHAR* GetType(WIN32_FIND_DATA fd)
{
	WCHAR *dot = new WCHAR[30];
	int vt = wcsrchr(fd.cFileName, L'.') - fd.cFileName;
	int len = wcslen(fd.cFileName);
	if (vt < 0 || vt >= len) //Nếu không tìm thấy
		return L"UNKNOWN";
	else
		wcscpy_s(dot,30,GetTypeFile(fd.cFileName));
	WCHAR *tail = new WCHAR[len - vt + 1];
	for (int i = 0; i < len - vt; i++)
		tail[i] = fd.cFileName[vt + i];
	tail[len - vt] = 0;
	if (!_wcsicmp(tail, L".htm") || !_wcsicmp(tail, L".html") || !_wcsicmp(tail, L".URL"))
		return L"Web page";
	WCHAR pszOut[256];
	HKEY hKey;
	DWORD dwType = REG_SZ;
	DWORD dwSize = 256;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, tail, 0, KEY_READ, &hKey))
	{
		RegCloseKey(hKey);
		return dot;
	}
	if (RegQueryValueEx(hKey, NULL, NULL, &dwType, (PBYTE)pszOut, &dwSize))
	{
		RegCloseKey(hKey);
		return dot;
	}
	RegCloseKey(hKey);
	WCHAR *pszPath = new WCHAR[1000];
	dwSize = 1000;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszOut, 0, KEY_READ, &hKey))
	{
		RegCloseKey(hKey);
		return dot;
	}

	if (RegQueryValueEx(hKey, NULL, NULL, &dwType, (PBYTE)pszPath, &dwSize))
	{
		RegCloseKey(hKey);
		return dot;
	}
	RegCloseKey(hKey);
	return pszPath;
}
//Load Icon
void LoadIcon()
{
	hmlbig = ImageList_Create(32, 32, ILC_MASK | ILC_COLOR16, 1, 1);
	hmlsmall = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR32, 1, 1);

	s = ExtractIconW(hInst, L"%SystemRoot%\\system32\\shell32.dll", 15);
	ImageList_AddIcon(hmlsmall, s);//1: PC
	s = ExtractIconW(hInst, L"%SystemRoot%\\system32\\shell32.dll", 4);
	ImageList_AddIcon(hmlsmall, s);//1: Folder
}
//Thanh statusbar
void StatusBarItem()
{
	WCHAR numitem[11];
	int num = ListView_GetItemCount(hWndListView);
	_itow_s(num, numitem, 10);
	if (num > 1)
		wcscat_s(numitem, L" items");
	else
		wcscat_s(numitem, L" item");
	SendMessage(hWndStatusBar, SB_SETTEXT, 0, (LPARAM)numitem);
	SendMessage(hWndStatusBar, SB_SETTEXT, 1, 0);
	SendMessage(hWndStatusBar, SB_SETTEXT, 2, 0);
}
//Kiem tra con kieu shell
int CheckChild(LPSHELLFOLDER father, LPITEMIDLIST root)
{
	LPSHELLFOLDER child = NULL;
	father->BindToObject(root, NULL, IID_IShellFolder, (LPVOID*)&child);
	if (child == NULL)
		return 0;
	LPENUMIDLIST enumList = NULL;
	HRESULT hr = child->EnumObjects(NULL, SHCONTF_FOLDERS, &enumList);
	if (enumList == NULL)
		return 0;
	LPITEMIDLIST pidl = NULL;
	hr = enumList->Next(1, &pidl, NULL);
	if (hr != S_OK)
		return 0;
	return 1;
}
//Thay doi cach liet ke
void ChangeView(LONG style)
{
	LONG notchoose = ~(LVS_REPORT | LVS_LIST | LVS_ICON | LVS_SMALLICON);
	SetWindowLong(hWndListView, GWL_STYLE, GetWindowLong(hWndListView, GWL_STYLE)&notchoose | style);
	if (LVS_ICON == style)
		bigIcon = 1;
	else
		bigIcon = 0;
	ListView_DeleteAllItems(hWndListView);
	LoadList(hWndListView, address);
	SetFocus(hWndListView);
}