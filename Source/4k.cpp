
/*
4k-Example.
A simple demo example to test 4k.
*/


#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#include "4k.h"


// Message handler:

LRESULT CALLBACK
WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}


// Initialization and shutdown:

bool
Initialize () {

    // no WinMain so get the module handle:
    g_hInstance = GetModuleHandle(NULL);
    if (g_hInstance == NULL)
        return false;

    // register the window class:
    WNDCLASS wc;

    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = WINDOW_TITLE;

    if (RegisterClass(&wc) == 0)
        return false;

    // create the window:
    g_hWnd = CreateWindow(
        WINDOW_TITLE,                                             // class name
        WINDOW_TITLE,                                             // title
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // style
        CW_USEDEFAULT, CW_USEDEFAULT,                             // position
        CW_USEDEFAULT, CW_USEDEFAULT,                             // size
        NULL,                                                     // no parent
        NULL,                                                     // no menu
        g_hInstance,                                              // instance
        NULL                                                      // no special
    );

    if (g_hWnd == NULL)
        return false;

    // setup double buffering:
    g_hDC = GetDC(g_hWnd);
    if (g_hDC == NULL)
        return false;

    g_hMemDC = CreateCompatibleDC(g_hDC);
    if (g_hMemDC == NULL)
        return false;

    g_hMemBMP = CreateCompatibleBitmap(g_hDC, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (g_hMemBMP == NULL)
        return false;

    SelectObject(g_hMemDC, g_hMemBMP);

    // colors used to draw the effect:
    g_hColor1 = CreateSolidBrush(RGB(184, 178, 251));
    g_hColor2 = CreateSolidBrush(RGB(98, 72, 124));

    if (g_hColor1 == NULL || g_hColor2 == NULL)
        return false;

    return true;
}


void
Shutdown (UINT uExitCode) {

    // release resources:
    if (g_hColor2 != NULL)
        DeleteObject(g_hColor2);

    if (g_hColor1 != NULL)
        DeleteObject(g_hColor1);

   if (g_hMemBMP != NULL)
        DeleteObject(g_hMemBMP);

    if (g_hMemDC != NULL)
        DeleteDC(g_hMemDC);

    if (g_hDC != NULL)
        ReleaseDC(g_hWnd, g_hDC);

    // destroy the window and unregister the class:
    if (g_hWnd != NULL)
        DestroyWindow(g_hWnd);

    WNDCLASS wc;
    if (GetClassInfo(g_hInstance, WINDOW_TITLE, &wc) != 0)
        UnregisterClass(WINDOW_TITLE, g_hInstance);

    // without WinMainCRTStartup() we must exit the process ourselves:
    ExitProcess(uExitCode);
}


 // A simple effect done using GDI rectangles:

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

// Pixels between each square and the next:

#define SQUARE_THICKNESS 30

// A distance from the center of the window that is guaranteed
// to (at least) reach the border in any direction. We add one pixel
// to account for odd resolutions:

#define BORDER_DISTANCE (1 + (MAX(WINDOW_WIDTH, WINDOW_HEIGHT) / 2))

// Window center:

#define CENTER_X (WINDOW_WIDTH / 2)
#define CENTER_Y (WINDOW_HEIGHT / 2)

// The animation loops for two full squares of different colors
// in either direction:

#define REVERSE_DISPLACEMENT 0

#if REVERSE_DISPLACEMENT
    #define DISPLACEMENT_START (SQUARE_THICKNESS * 2)
    #define DISPLACEMENT_END 0
    #define DISPLACEMENT_STEP -1
#else
    #define DISPLACEMENT_START 0
    #define DISPLACEMENT_END (SQUARE_THICKNESS * 2)
    #define DISPLACEMENT_STEP 1
#endif


int displacement = DISPLACEMENT_START;

void
Update () {

    // starting at the largest square that reaches the border distance
    // draw smaller and smaller squares until we get to the center:
    for (int i = 1, size = BORDER_DISTANCE; ; i++, size -= SQUARE_THICKNESS) {
        int offset = size + displacement;

        // stop at the center:
        if (offset < 1)
            break;

        // GDI allows us to draw outside the window, but let's be polite...
        RECT r = {
            MAX(CENTER_X - offset, 0),
            MAX(CENTER_Y - offset, 0),
            MIN(CENTER_X + offset, WINDOW_WIDTH),
            MIN(CENTER_Y + offset, WINDOW_HEIGHT),
        };

        FillRect(g_hMemDC, &r, i % 2 == 0 ? g_hColor1 : g_hColor2);
    }

    // back to the start?
    displacement += DISPLACEMENT_STEP;
    if (displacement == DISPLACEMENT_END)
        displacement = DISPLACEMENT_START;
}


void
Loop () {
    MSG msg;
    bool done = false;

    // this game loop is far from ideal since rendering/logic are tied
    // and timeGetTime() and Sleep() are not precise, but it is
    // small and works well enough for simple 4k games/demos

    // for a better game loop see:
    // <http://gafferongames.com/game-physics/fix-your-timestep>

    while (!done) {
        DWORD dwStart = timeGetTime();

        // poll windows events:
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
            if (msg.message == WM_QUIT)
                done = true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // update and copy the back buffer:
        Update();
        BitBlt(g_hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, g_hMemDC, 0, 0, SRCCOPY);

        // sleep until next step:
        DWORD dwDelta = timeGetTime() - dwStart;
        if (dwDelta < STEP_RATE) {
            Sleep(STEP_RATE - dwDelta);
        }
    }
}


// A helper to resize the window with respect to the client area:

void
ResizeClientWindow (HWND hWnd, UINT uWidth, UINT uHeight) {
    RECT rcClient, rcWindow;

    GetClientRect(hWnd, &rcClient);
    GetWindowRect(hWnd, &rcWindow);

    MoveWindow(hWnd,
               rcWindow.left,
               rcWindow.top,
               uWidth + (rcWindow.right - rcWindow.left) - rcClient.right,
               uHeight + (rcWindow.bottom - rcWindow.top) - rcClient.bottom,
               FALSE);
}


// Entry point:

void
EntryPoint () {
    if (!Initialize()) {
        MessageBox(NULL, "Initialization failed.", "Error", MB_OK | MB_ICONERROR);
        Shutdown(1);
    }

    ResizeClientWindow(g_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT);
    ShowWindow(g_hWnd, SW_SHOW);
    Loop();
    Shutdown(0);
}

