// Copywrite Scangine OOD 
// www.scangine.com
// License - GNU GPLv3 
//

#include "framework.h"
#include "DummyRepeater.h"

#define MAX_LOADSTRING 100

// Global Variables:
HANDLE hThread = NULL;
HWND TEXTBOX = NULL;
HWND TEXTBOX2 = NULL;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL RUN_MUTHERFUCKER = false;
BOOL RECORD_LOOP = false;

HHOOK hhkLowLevelKybd = NULL;
HHOOK hhkLowLevelMouse = NULL;

LONG startMouseX = 0;
LONG startMouseY = 0;

static int ALPHA_PERCENT = 100;
static bool RECORDING = false;


HANDLE hookmutex;


static const WORD TYPE_KEY = 0;
static const WORD TYPE_MOUSE = 1;

static LONG screenWidth = 0;
static LONG screenHeight = 0;

struct InputEvent
{
    WORD type;
    DWORD time;

    DWORD flags;

    //keyboard
    WORD vk;
    WORD sc;


    //mouse 
    LONG x;
    LONG y;
    DWORD mouseData;

    InputEvent* next;

};

static InputEvent* event = NULL;
static InputEvent* firstevent = NULL;

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
    LoadStringW(hInstance, IDC_DUMMYREPEATER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DUMMYREPEATER));

    MSG msg;

    hookmutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex


    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }



    if (NULL != hhkLowLevelKybd)
    {
        UnhookWindowsHookEx(hhkLowLevelKybd);
        hhkLowLevelKybd = NULL;
    }

    if (NULL != hhkLowLevelMouse)
    {
        UnhookWindowsHookEx(hhkLowLevelMouse);
        hhkLowLevelMouse = NULL;
    }

    clearRecording();
    return (int)msg.wParam;
}


bool isCtrlPressed()
{
    int vk = VK_CONTROL;//control key
//    int vsc = MapVirtualKeyA(vk, MAPVK_VSC_TO_VK);
//    int e0 = ((vsc >> 8) & 0xff) == 0xe0;   // e0 ?
//    int e1 = ((vsc >> 8) & 0xff) == 0xe1;   // e1 ?
//    int sc = vsc & 0xff;                    // mask eventual scan code flags
    DWORD state = GetKeyState(vk);
    bool pressed = (state & 0xc0000000) != 0;    // check top bits to know if a key was pressed or released
    return pressed;
}

void sendControlUp()
{
    int vk = VK_CONTROL;//control key
    int vsc = MapVirtualKeyA(vk, MAPVK_VSC_TO_VK);
    int sc = vsc & 0xff;                    // mask eventual scan code flags
    sendKeyEvent(vk, sc, KEYEVENTF_KEYUP);
}

void onGlobalKeyEvent(
    DWORD scancode,         // (virtual) scan code
    bool    e0,         // is the E0 extended flag set?
    bool    e1,         // is the E1 extended flag set?
    bool    keyup,      // is the event a key press (0) or release (1)
    DWORD virtualKey          // virtual key associated to the key event
)
{
    if ((virtualKey == '4') && isCtrlPressed())
    {
        stopLoop();
        return;
    }

    if ((virtualKey == '3') && isCtrlPressed())
    {
        startLoop();
        return;
    }

    if ((virtualKey == '1') && isCtrlPressed())
    {
        ALPHA_PERCENT -= 10;
        if (ALPHA_PERCENT < 5)
            ALPHA_PERCENT = 5;

        makeWinTrransparent();
        return;
    }


    if ((virtualKey == '2') && isCtrlPressed())
    {
        ALPHA_PERCENT += 10;
        if (ALPHA_PERCENT > 100)
            ALPHA_PERCENT = 100;

        makeWinTrransparent();
        return;
    }

    if (keyup && (virtualKey == '7') && isCtrlPressed())
    {
        startRecording();
        return;
    }

    if ((virtualKey == '8') && isCtrlPressed())
    {
        stopRecording();
        return;
    }

    if (keyup && (virtualKey == '9') && isCtrlPressed())
    {
        playRecording();
        return;
    }

    if (keyup && (virtualKey == '0') && isCtrlPressed())
    {
        stopLoop();
        stopRecording();
        sendControlUp();
        return;
    }


}


DWORD mouseWPARAMToFlags(WPARAM wparam)
{
    DWORD flags = 0;

    if (wparam == WM_MOUSEMOVE)
    {
        flags |= MOUSEEVENTF_MOVE;
        return flags;
    }

    if (wparam == WM_LBUTTONDOWN)
    {
        flags |= MOUSEEVENTF_LEFTDOWN;
        return flags;
    }

    if (wparam == WM_LBUTTONUP)
    {
        flags |= MOUSEEVENTF_LEFTUP;
        return flags;
    }

    if (wparam == WM_RBUTTONDOWN)
    {
        flags |= MOUSEEVENTF_RIGHTDOWN;
        return flags;
    }

    if (wparam == WM_RBUTTONUP)
    {
        flags |= MOUSEEVENTF_RIGHTUP;
        return flags;
    }

    if (wparam == WM_MBUTTONDOWN)
    {
        flags |= MOUSEEVENTF_MIDDLEDOWN;
        return flags;
    }

    if (wparam == WM_MBUTTONUP)
    {
        flags |= MOUSEEVENTF_MIDDLEUP;
        return flags;
    }

    if (wparam == WM_MOUSEWHEEL)
    {
        flags |= MOUSEEVENTF_WHEEL;
        return flags;
    }

    if (wparam == WM_XBUTTONDOWN)
    {
        flags |= MOUSEEVENTF_XDOWN;
        return flags;
    }

    if (wparam == WM_XBUTTONUP)
    {
        flags |= MOUSEEVENTF_XUP;
        return flags;
    }

    if (wparam == WM_MOUSEHWHEEL)
    {
        flags |= MOUSEEVENTF_HWHEEL;
        return flags;
    }

    return flags;
}

void createInputEvent()
{
    InputEvent* newevent = new InputEvent();
    ZeroMemory(newevent, sizeof(&newevent));
    newevent->next = NULL;

    if (firstevent == NULL)
    {
        firstevent = newevent;
        event = firstevent;
    }
    else if (event != NULL)
    {
        event->next = newevent;
    }
    event = newevent;
    event->time = GetTickCount();

    
}

DWORD keyboardFlagsTransit(DWORD llflags)
{
    DWORD flags = 0;
    if (llflags & LLKHF_UP)
    {
        flags |= KEYEVENTF_KEYUP;
    }

    if (llflags & LLKHF_EXTENDED)
    {
        flags |= KEYEVENTF_EXTENDEDKEY;
    }

    return flags;
}

LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wparm, LPARAM lparm)
{


    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lparm;



    onGlobalKeyEvent(p->scanCode,
        p->flags & LLKHF_EXTENDED,
        0,
        p->flags & LLKHF_UP,
        p->vkCode
    );

    if ((p->vkCode == VK_CONTROL) || (p->vkCode == VK_LCONTROL) || (p->vkCode == VK_RCONTROL))
    {
        return CallNextHookEx(NULL, code, wparm, lparm);
    }


    DWORD waitResult = WaitForSingleObject(
        hookmutex,    // handle to mutex
        INFINITE);  // no time-out interval


    if (RECORDING)
    {
        if (!isCtrlPressed())
        {
            createInputEvent();

            event->type = TYPE_KEY;
            event->vk = p->vkCode;
            event->sc = p->scanCode;
            event->flags = keyboardFlagsTransit(p->flags);
        }
    }

    ReleaseMutex(hookmutex);

    return CallNextHookEx(NULL, code, wparm, lparm);
}

LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wparam, LPARAM lparam)
{
    DWORD waitResult = WaitForSingleObject(
        hookmutex,    // handle to mutex
        INFINITE);  // no time-out interval

    if (RECORDING)
    {
        //MOUSEHOOKSTRUCT* p = (MOUSEHOOKSTRUCT*)lparam;
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lparam;
        if (p != NULL)
        {
            DWORD flags = mouseWPARAMToFlags(wparam);
            if (flags != 0)
            {
                createInputEvent();

            

                event->type = TYPE_MOUSE;
                event->x = p->pt.x;
                event->y = p->pt.y;
                event->flags = flags;

                event->mouseData = 0;
                if ((event->flags & MOUSEEVENTF_WHEEL) || (event->flags & MOUSEEVENTF_HWHEEL))
                    event->mouseData = HIWORD(p->mouseData);
            }

        }
    }

    ReleaseMutex(hookmutex);
    return CallNextHookEx(NULL, code, wparam, lparam);
}


void clearRecording()
{
    DWORD waitResult = WaitForSingleObject(
        hookmutex,    // handle to mutex
        INFINITE);  // no time-out interval

    InputEvent* e;
    while (firstevent != NULL)
    {
        e = firstevent;
        firstevent = firstevent->next;
        delete e;
    }

    event = NULL;
    ReleaseMutex(hookmutex);
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

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DUMMYREPEATER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DUMMYREPEATER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, 0, 500, 270, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }



    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        //Sethook
        if (NULL == hhkLowLevelKybd)
        {
            hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
            hhkLowLevelMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);

        }


        //Create buttons
        HWND stopBtn = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Stop",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            150,         // y position 
            100,        // Button width
            50,        // Button height
            hWnd,     // Parent window
            (HMENU)1,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);

        HWND startBtn = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Start",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            120,         // x position 
            150,         // y position 
            100,        // Button width
            50,        // Button height
            hWnd,     // Parent window
            (HMENU)2,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);


        HWND hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            10, 18, 210, 20, hWnd, (HMENU)111, NULL, NULL);
        SetWindowText(hwnd_label, L"Repeat:");

        TEXTBOX = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE,
            10, 40, 210, 30, hWnd, (HMENU)3, NULL, NULL);

        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            10, 90, 50, 30, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Delay:");
        TEXTBOX2 = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER,
            60, 90, 160, 30, hWnd, (HMENU)3, NULL, NULL);

        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 18, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+3  START char loop");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 38, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+4  STOP char loop");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 65, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+7  RECORD");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 85, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+8  STOP RECORDING");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 105, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+9  PLAY RECORD");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 125, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+0  STOP RECPLAY");

        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 155, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+1  + transparency");
        hwnd_label = CreateWindowEx(0, L"static", L"", WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT,
            260, 175, 200, 20, hWnd, (HMENU)112, NULL, NULL);
        SetWindowText(hwnd_label, L"Ctrl+2  - transparency");

        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);

        break;
    }
    /*   case WM_CTLCOLORSTATIC:                             // Do not allow DefWindowProc() to process this message!
       {
           HDC hdcStatic = (HDC)wParam;
           SetTextColor(hdcStatic, RGB(0, 0, 0));
           SetBkMode(hdcStatic, TRANSPARENT);
           return(INT_PTR)hBrush;
       }break;
   */
    case WM_COMMAND:
    {

        int wmId = LOWORD(wParam);



        // Parse the menu selections:
        switch (wmId)
        {

        case 1:
            stopLoop();
            break;

        case 2:
            startLoop();
            break;

        case IDM_ABOUT:
            //   startHack();
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:

        if (NULL != hhkLowLevelKybd)
        {
            UnhookWindowsHookEx(hhkLowLevelKybd);
            hhkLowLevelKybd = NULL;
        }

        if (NULL != hhkLowLevelMouse)
        {
            UnhookWindowsHookEx(hhkLowLevelMouse);
            hhkLowLevelMouse = NULL;
        }

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


int getKeyScancode(char key)
{
    switch (key)
    {
    case '1':
        return 0x02;
    case '2':
        return 0x03;
    case '3':
        return 0x04;
    case '4':
        return 0x05;
    case '5':
        return 0x06;
    case '6':
        return 0x07;
    case '7':
        return 0x08;
    case '8':
        return 0x09;
    case '9':
        return 0x0A;
    case '0':
        return 0x0B;
    case '-':
    case '_':
        return 0x0C;
    case '=':
        return 0x0D;
    case '\t':
        return 0x0F;
    case 'Q':
    case 'q':
        return 0x10;
    case 'W':
    case 'w':
        return 0x11;
    case 'E':
    case 'e':
        return 0x12;
    case 'R':
    case 'r':
        return 0x13;
    case 'T':
    case 't':
        return 0x14;
    case 'Y':
    case 'y':
        return 0x15;
    case 'U':
    case 'u':
        return 0x16;
    case 'I':
    case 'i':
        return 0x17;
    case 'O':
    case 'o':
        return 0x18;
    case 'P':
    case 'p':
        return 0x19;
    case '[':
    case '{':
        return 0x1A;
    case '}':
    case ']':
        return 0x1B;
    case '\n':
        return 0x1C;
    case 'A':
    case 'a':
        return 0x1E;
    case 'S':
    case 's':
        return 0x1F;
    case 'D':
    case 'd':
        return 0x20;
    case 'F':
    case 'f':
        return 0x21;
    case 'G':
    case 'g':
        return 0x22;
    case 'H':
    case 'h':
        return 0x23;
    case 'J':
    case 'j':
        return 0x24;
    case 'K':
    case 'k':
        return 0x25;
    case 'L':
    case 'l':
        return 0x26;
    case ';':
    case ':':
        return 0x27;
    case '\'':
    case '\"':
        return 0x28;
    case '`':
    case '~':
        return 0x29;
    case '\\':
    case '|':
        return 0x2B;
    case 'Z':
    case 'z':
        return 0x2C;
    case 'X':
    case 'x':
        return 0x2D;
    case 'C':
    case 'c':
        return 0x2E;
    case 'V':
    case 'v':
        return 0x2F;
    case 'B':
    case 'b':
        return 0x30;
    case 'N':
    case 'n':
        return 0x31;
    case 'M':
    case 'm':
        return 0x32;
    case '<':
    case ',':
        return 0x33;
    case '>':
    case '.':
        return 0x34;
    case '?':
    case '/':
        return 0x35;
    case ' ':
        return 0x39;

    }

    return 0;
}


int getKeyVK(char key)
{
    switch (key)
    {
    case '1':
        return '1';
    case '2':
        return '2';
    case '3':
        return '3';
    case '4':
        return '4';
    case '5':
        return '5';
    case '6':
        return '6';
    case '7':
        return '7';
    case '8':
        return '8';
    case '9':
        return '9';
    case '0':
        return '0';
    case '-':
    case '_':
        return VK_SUBTRACT;
    case '=':
        return 0x0D;
    case '\t':
        return VK_TAB;
    case 'Q':
    case 'q':
        return 'Q';
    case 'W':
    case 'w':
        return 'W';
    case 'E':
    case 'e':
        return 'E';
    case 'R':
    case 'r':
        return 'R';
    case 'T':
    case 't':
        return 'T';
    case 'Y':
    case 'y':
        return 'Y';
    case 'U':
    case 'u':
        return 'U';
    case 'I':
    case 'i':
        return 'I';
    case 'O':
    case 'o':
        return 'O';
    case 'P':
    case 'p':
        return 'P';
    case '[':
    case '{':
        return VK_OEM_4;
    case '}':
    case ']':
        return VK_OEM_6;
    case '\n':
        return VK_RETURN;
    case 'A':
    case 'a':
        return 'A';
    case 'S':
    case 's':
        return 'S';
    case 'D':
    case 'd':
        return 'D';
    case 'F':
    case 'f':
        return 'F';
    case 'G':
    case 'g':
        return 'G';
    case 'H':
    case 'h':
        return 'H';
    case 'J':
    case 'j':
        return 'J';
    case 'K':
    case 'k':
        return 'K';
    case 'L':
    case 'l':
        return 'L';
    case ';':
    case ':':
        return VK_OEM_1;
    case '\'':
    case '\"':
        return VK_OEM_7;
    case '`':
    case '~':
        return VK_OEM_3;
    case '\\':
    case '|':
        return VK_OEM_5;
    case 'Z':
    case 'z':
        return 'Z';
    case 'X':
    case 'x':
        return 'X';
    case 'C':
    case 'c':
        return 'C';
    case 'V':
    case 'v':
        return 'V';
    case 'B':
    case 'b':
        return 'B';
    case 'N':
    case 'n':
        return 'N';
    case 'M':
    case 'm':
        return 'M';
    case '<':
    case ',':
        return VK_OEM_COMMA;
    case '>':
    case '.':
        return VK_DECIMAL;
    case '?':
    case '/':
        return VK_OEM_2;
    case ' ':
        return VK_SPACE;

    }

    return 0;
}


void sendKey(char key, int delay)
{
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));

    int scanCode = getKeyScancode(key);
    int vk = getKeyVK(key);

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = 0;

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    Sleep(2);

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

    uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(delay);
}


void playEvent(InputEvent* e)
{
    if (NULL == e)
        return;

    if (e->type == TYPE_KEY)
    {
        sendKeyEvent(e->vk, e->sc, e->flags);
    }
    else if (e->type == TYPE_MOUSE)
    {
        sendMouseEvent(e->x, e->y, e->mouseData, e->flags, 0);
    }
 
}

void sendKeyEvent(WORD vk, WORD scanCode, DWORD flags)
{
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = flags;

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}


void sendMouseEvent(LONG dx, LONG dy, DWORD mouseData, DWORD flags, DWORD time)
{
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));

 //   SetCursorPos(dx, dy);

    inputs[0].type = INPUT_MOUSE;
 //   if (flags & MOUSEEVENTF_MOVE)
 //   {
        inputs[0].mi.dx = (dx * 65535) / screenWidth;
        inputs[0].mi.dy = (dy * 65535) / screenHeight;
  //  }
  //  else {
  //      inputs[0].mi.dx = 0;// dx;
  //      inputs[0].mi.dy = 0;// dy;
  //  }

    inputs[0].mi.mouseData = mouseData;
    inputs[0].mi.dwFlags = flags | MOUSEEVENTF_ABSOLUTE;
    inputs[0].mi.time = time;
    inputs[0].mi.dwExtraInfo = NULL;


    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}




void tupHack(LPSTR str, int delay)
{
    LPSTR text = str;
    if (str == NULL)
        return;
    CHAR c = *text;
    while (c != 0)
    {
        sendKey(c, delay);
        text += 1;
        c = *text;
    }

}

void stopRecording()
{
    RECORDING = false;
    Sleep(100);
}

void startRecording()
{
    stopLoop();
    Sleep(100);
    if (RECORDING)
    {
        stopRecording();
    }

    clearRecording();
    RECORDING = true;

    POINT pos;
    if (!GetCursorPos(&pos))
    {
        startMouseX = 0;
        startMouseY = 0;
    }
    else {
        startMouseX = pos.x;
        startMouseY = pos.y;
    }
}

void playRecording()
{
    stopLoop();
    Sleep(100);
    if (RECORDING)
    {
        stopRecording();
    }

    RECORD_LOOP = true;
    startLoop();
}

void startLoop()
{
    if (RUN_MUTHERFUCKER)
        return;

    if (NULL != hThread)
        return;

    RUN_MUTHERFUCKER = true;
    DWORD threadID = 0;

    hThread = CreateThread(
        NULL,    // Thread attributes
        0,       // Stack size (0 = use default)
        workerThreadFunction, // Thread start address
        NULL,    // Parameter to pass to the thread
        0,       // Creation flags
        NULL);   // Thread id
    if (hThread == NULL)
    {
        // Thread creation failed.
        // More details can be retrieved by calling GetLastError()
        return;
    }


}

void stopLoop()
{
    RUN_MUTHERFUCKER = false;

    // Wait for thread to finish execution
    WaitForSingleObject(hThread, INFINITE);

    // Thread handle must be closed when no longer needed
    CloseHandle(hThread);
    hThread = NULL;
}

DWORD WINAPI workerThreadFunction(LPVOID lpParam)
{

    if (RECORD_LOOP)
    {
        if (NULL == firstevent)
            return 0;

        DWORD starttime;
        DWORD time;
        InputEvent* e = NULL;


        SetCursorPos(startMouseX, startMouseY);

        while (RUN_MUTHERFUCKER)
        {
            e = firstevent;
            starttime = GetTickCount();
            while (e != NULL)
            {
                time = GetTickCount();
                if ((e->time - firstevent->time) <= (time - starttime))
                {
                    playEvent(e);
                    e = e->next;
                }
                else
                {
                    Sleep(1);
                }
            }
        }
    }
    else
    {
        LPSTR str = GetHText();
        LPSTR strTime = GetDelayText();
        int delay = atoi(strTime);
        if (delay <= 0)
            delay = 100;

        while (RUN_MUTHERFUCKER)
        {
            tupHack(str, delay);
        }

        delete[] str;
        delete[] strTime;
    }

    RECORD_LOOP = false;
    return 0;
}

LPSTR GetHText()
{
    int length;
    length = GetWindowTextLength(TEXTBOX) + 1;
    LPSTR str = new CHAR[length];
    GetWindowTextA(TEXTBOX, str, length);
    return str;
}

LPSTR GetDelayText()
{
    int length;
    length = GetWindowTextLength(TEXTBOX2) + 1;
    LPSTR str = new CHAR[length];
    GetWindowTextA(TEXTBOX2, str, length);
    return str;
}

void makeWinTrransparent()
{
    HWND win = GetForegroundWindow();
    //    ShowWindow(win, SW_MINIMIZE);
    SetWindowLong(win, GWL_EXSTYLE, GetWindowLong(win, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(win, 0, (255 * ALPHA_PERCENT) / 100, LWA_ALPHA);

}